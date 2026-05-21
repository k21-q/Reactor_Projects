#pragma once
//事件监控模块
/*
  Poller模块：描述符事件监控模块
     1.针对channel添加/修改监控
     2.针对channel移除监控
     3.针对所有描述符开始事件监控，并返回所有就绪的channel
*/

#include<vector>
#include<unordered_map>
#include<memory>

namespace net{
    class Channel;
    class Timestamp;
    class Poller;
    using PollerPtr=std::shared_ptr<Poller>;


    static int createEpoll();
    const static int EPOLL_TIMEOUT=1000;
    const static int DEFAULT_EVENT_SIZE=16;  //默认的事件数组大小
    const static uint32_t KNoneEvent=0;  //不监控任何事件  无事件  
    const static uint32_t KReadEvent=EPOLLIN|EPOLLPRT;  //监控读  读事件  EPOLLPRT:表示对端关闭连接了，监控这个事件就能够及时发现对端关闭连接了
    const static uint32_t KWriteEvent=EPOLLOUT;  //监控写  写事件

    //muduo库中将描述符的监控1+管理分开了
     // 移除事件监控： 只 epoll_ctl解除监控，但不移除管理
     // 移除监控管理： 即解除监控，也移除管理

    //抽象类，主要描述要实现的功能，具体实现在派生类中

    class Poller{ 
        public:
            Poller()=default;
            virtual ~Poller()=default;
           //对所有描述符进行监控，并获取就绪的描述符对应的channel
            virtual Timestamp wait(std::vector<Channel*> &actives)=0; //获取到所有的活跃就绪事件，通过就绪的描述符，找到channel
            // 要实现新增 以及 修改事件   添加/修改
            virtual void updateChannel(Channel* channel)=0;  //  获取到描述符，然后获取到要监控的事件，来改变他
            //解除并移除监控   并移除管理
            virtual void removeChannel(Channel* channel)=0;  

            static PollerPtr defultPoller();   //创建默认Poller对象的
        protected:
            //map 红黑树 和unordered_map 哈希表 不一样
            std::unordered_map<int,Channel*> _channels;  //文件描述符和Channel有一个映射关系，这样就能够找到Channel了
        
    };
    class EpollPoller:public Poller{
        public:
            EpollPoller();
            virtual ~EpollPoller();
           //对所有描述符进行监控，并获取就绪的描述符对应的channel
            virtual Timestamp wait(std::vector<Channel*> &actives) override; //获取到所有的活跃就绪事件，通过就绪的描述符，找到channel
            // 要实现新增 以及 修改事件   添加/修改
            virtual void updateChannel(Channel* channel) override;  //  获取到描述符，然后获取到要监控的事件，来改变他
            //解除并移除监控   并移除管理
            virtual void removeChannel(Channel* channel) override;  
        private:
            const char* eventStr(int op);
            void update(int op,Channel* channel);  //对描述符进行操作，op是操作类型，channel是要操作的对象
        private:
            int _epfd;  //epoll的文件描述符 
            std::vector<struct epoll_event> _evs;  //预先分配一个大小为16的事件数组，来存储就绪事件
    };
}