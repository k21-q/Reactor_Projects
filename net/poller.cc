#include"poller.h"
#include "timestamp.h"
#include"log.h"
#include"channel.h"
#include<sys/epoll.h>
#include<cstring>
#include<errno.h>
#include<unistd.h>
#include<cassert>


namespace net{
  //:: 全局作用域的说明，作用
    static int createEpoll(){
        //EPOLL_CLOEXEC: 该标志表示在调用exec函数族时，自动关闭该文件描述符，避免子进程继承父进程的epoll文件描述符，导致资源泄漏
        //创建子进程时，关闭描述符，防止子进程复制描述符的相关信息
        int fd=::epoll_create1(EPOLL_CLOEXEC);  //创建一个epoll对象，返回其文件描述符
        if(fd<0){
            LOG_FATAL("创建epoll失败: %s",strerror(errno));
        }
        return fd;
    }


    PollerPtr Poller::defultPoller(){
        std::shared_ptr<Poller> ptr=std::make_shared<EpollPoller>();  //智能指针管理的Poller对象，返回一个智能指针
        return ptr;
        //return std::make_shared<EpollPoller>();
    } 

    //针对成员进行初始化： _epfd
    EpollPoller::EpollPoller():
          _epfd(createEpoll()),//创建fd
          _evs(DEFAULT_EVENT_SIZE){}  //预先分配一个大小为16的事件数组，来存储就绪事件
          
    EpollPoller::~EpollPoller(){ ::close(_epfd);}    //关闭epoll


    //对所有描述符进行监控，并获取就绪的描述符对应的channel//获取到所有的活跃就绪事件，通过就绪的描述符，找到channel
    Timestamp EpollPoller::wait(std::vector<Channel*> &actives){
        //int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
        actives.clear();  //清空活跃事件数组
        int ret=epoll_wait(_epfd,&_evs[0],_evs.size(),EPOLL_TIMEOUT); //等待事件发生，返回就绪事件的数量
        if(ret<0){
            LOG_ERROR("epoll_wait 出错: %s",strerror(errno));
            return;
        }else if(ret==0){   //超时了
            LOG_DEBUG("epoll_wait 超时了");
            return;
        }
        for(int i=0;i<ret;++i){
            Channel* channel = (Channel*)_evs[i].data.ptr; //通过就绪的描述符，找到channel      //////
            channel->setRevents(_evs[i].events);  //设置就绪事件
        }
        if(ret==_evs.size()){  //意味着就绪事件数组打满了
            _evs.resize(_evs.size()*2);  //将事件数组的大小扩大一倍
        }
    }

    const char* EpollPoller::eventStr(int op){
        switch(op){
            case EPOLL_CTL_ADD: return "EPOLL_CTL_ADD";
            case EPOLL_CTL_MOD: return "EPOLL_CTL_MOD";
            case EPOLL_CTL_DEL: return "EPOLL_CTL_DEL";       
        } 
        LOG_FATAL("epoll监控类型错误:%d",op);
    
    }
     void EpollPoller::update(int op,Channel* channel){
        //实际实现epoll_ctl相关的操作
        // 1.获取channel中的fd和events  描述符和要监控的事件
        int fd=channel->fd();
       // int events=channel->events();
        // 2.使用epoll_ctl进行实践操作
        struct epoll_event ev;
        ev.data.ptr=channel;  //将channel对象的地址存储在事件结构体中，这样在事件就绪时就可以通过这个指针找到对应的channel对象
        ev.events=channel->events();  //获取要监控的事件
        int ret=epoll_ctl(_epfd,op,fd,&ev);  //对描述符进行操作，op是操作类型，channel是要操作的对象 &ev 事件的结构
        if(ret<0){
            LOG_ERROR("epoll_ctl 出错: %s-%s",eventStr(op),strerror(errno));
        }
     }
    // 要实现新增 以及 修改事件   添加/修改         //  获取到描述符，然后获取到要监控的事件，来改变他
    void EpollPoller::updateChannel(Channel* channel){
        //要多channel中的描述符进行，根据不同的状态进行添加/修改/解除epoll监控操作
        //KNew:新增监控&新增管理； 
        //KAdded:修改要监控的事件;  
        //KDeleted:表示描述符没有监控任何事件，但是以前添加过管理，新增监控
        ChannelState state=channel->state();  //获取channel的状态
        int fd=channel->fd();
        if(state==KNew||state==KDeleted){  //新增监控
            if(state==KNew){  //添加监控&管理
                assert(_channels.find(fd)==_channels.end());  //断言：在管理中找不到这个描述符，说明是新添加的
                _channels.insert(std::make_pair(fd,channel));  //添加管理
            }
            else{
                assert(_channels.find(fd)!=_channels.end());  //断言：在管理中找得到这个描述符，说明是之前添加过的
                assert(_channels[fd]==channel);  //断言：在管理中找到的这个描述符对应的channel对象就是当前这个channel对象
            }
            update(EPOLL_CTL_ADD,channel);  //添加监控
            channel->setState(KAdded);  //修改状态为已添加监控
        }else{
            //监控中的描述符：若当前监控的事件是0，就代表要解除监控；否则就是修改要监控的事件
            if(channel->events()==KNoneEvent)  //没有事件监控 所以就要解除监控
            {
                update(EPOLL_CTL_DEL,channel);  //解除监控
                channel->setState(KDeleted);  //修改状态为已删除监控
            }else{
                update(EPOLL_CTL_MOD,channel);  //修改监控
            }  
         }
    }  
    //解除并移除监控   并移除管理
    void EpollPoller::removeChannel(Channel* channel){
        int fd=channel->fd();
        assert(_channels.find(fd)!=_channels.end());  //断言：在管理中找得到这个描述符，说明是之前添加过的
        assert(_channels[fd]==channel);  //断言：在管理中找到的这个描述符对应的channel对象就是当前这个channel对象
        //1.解除epoll事件监控
        if(channel->state()==KAdded){  //如果当前描述符是监控状态，就解除监控
            update(EPOLL_CTL_DEL,channel);  //解除监控
        }
        //2.解除channel管理
        _channels.erase(fd);  //根据描述符找到channel对象，并移除管理
        channel->setState(KNew);  //修改状态为新建状态

    }
}