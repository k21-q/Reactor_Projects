#pragma once
/*
  抽象事件处理器模块： 对描述符进行事件处理描述管理
  设计：
    1.要监控的描述符
    2.要监控的事件
    3.描述符事件就绪的事件
    4.针对时间如何处理的回调函数
*/
// 事件处理器模块
#include <stddef.h>
#include <iostream>
#include <functional>
#include <stdint.h>


// 事件回调函数
namespace net
{
    class Timestamp;
    class EventLoop;
    using ReadCallBack = std::function<void(Timestamp)>; // 读事件回调函数
    using EventCallBack = std::function<void()>;         // 其他事件回调函数
    enum ChannelState
    {
        // 描述符添加监控，有不同的状态，创建时没有状态，添加时才有
        KNew = 1, // 新建还未添加事件监控
        KAdded,   // 已添加事件监控
        KDeleted  // 已解除事件监控
    };
    class Channel
    {
    public:
        Channel(int fd, EventLoop* loop);
        ~Channel();
        int fd() { return _fd; }
        ChannelState state() { return _state; }
        void setState(ChannelState state) { _state = state; }
        uint32_t events() { return _events; }         // 获取要监控的事件
        void setRevents(uint32_t e) { _revents = e; } // 设置就绪事件

        void setTie(const std::shared_ptr<void> &obj)
        {
            _tied = true;
            _tie = obj;
        }

        void setReadCallBack(ReadCallBack cb) { _readCallback = std::move(cb); } // move移动
        void setWriteCallBack(EventCallBack cb) { _writeCallback = std::move(cb); }
        void setErrorCallBack(EventCallBack cb) { _errorCallback = std::move(cb); }
        void setCloseCallBack(EventCallBack cb) { _closeCallback = std::move(cb); }

        //判断当前文件描述符是否出去无事件监控状态
        bool isNoneEvent() const { return _events ==KNoneEvent;}
        //描述符是否监控写事件
        bool isWriting() const { return _events == KWriteEvent; }
        //描述符是否监控读事件
        bool isReading() const {return  _events == KReadEvent;} 

        //对当前描述符进行事件操作
        //启动读事件监控
        void enableReading(){
            //1.将当前描述符要监控的事件设置为读事件
            _events |=KReadEvent;
            //2.找到当前描述符分配的Poller，调用他的updateChannel(this)
            update();  //功能在update中实现，延迟实现
        }
       
        void enableWriting(){
            _events |= KWriteEvent;
            update();
        }

        void disableReading(){
          _events &=~KReadEvent;
          update();
        }
       
        void disableWriting(){
            _events &= ~KWriteEvent;;
            update();
        }
        
        void disableAll(){
            _events = KNoneEvent;
            update();

        } 
        // 总的事件处理函数
       void handleEvent(Timestamp recvTime)
       {
        if(_tied){
            if(_tie.lock()){  //能否获取到外部对象
                handleEventWithGuard(recvTime);
            }
        }else{
            //如果不需要关系外部的管理对象生命周期。则直接调用
            handleEventWithGuard(recvTime);
        }
       }


        void remove();  //解除监控，并移除管理

       
    private:

       void update();  //用于实际的调用poller，对描述符进行事件监控
 
      
       // 总的事件处理函数,任何就绪的事件就会调用这个函数  Poller类文件描述符监控后  会根据实际就绪的事件调用不同的类进行处理
       void handleEventWithGuard(Timestamp recvTime)
       {

         _eventHandling=true;  //正在事件处理中
        //一个文件描述符可能会同时触发多个事件

           //在这个函数中，根据实际就绪的事件，调用不同的回调函数进行事件处理
           //连接关闭事件：就绪了连接挂断事件，且当前没有新数据到来，直接调用关闭回调
           if(_revents & EPOLLHUP && !(_revents & KReadEvent)){
             if(_closeCallback) _closeCallback();
           }
            if(_revents & KReadEvent){
                if(_readCallback) _readCallback(recvTime);
            }
            
            if(_revents & KWriteEvent){
                if(_writeCallback) _writeCallback();
            }
            //错误事件
            if(_revents & EPOLLERR){
                if(_errorCallback) _errorCallback();
            }
            _eventHandling=false;  //事件处理完毕
       }

    private:
       EventLoop* _loop;  //Channel描述符所挂的事件循环
        int _fd;             // 要监控的事件
        ChannelState _state; // 事件状态
        uint32_t _events;    // 要监控的事件 //EPOLLIN:
        uint32_t _revents;   // 保存实际就绪的事件

        // 回调函数：拿到的就是函数指针，可以直接调用；
        // 可以说一下为什么使用的时回调函数而不是虚函数
        // 回调函数的好处是可以让用户自己定义事件处理函数，灵活性更高；而虚函数则需要用户继承Channel类并重写事件处理函数，增加了使用的复杂度

        bool _tied;
        // 观察者模式的一种特殊使用方式，通过它观察外部管理对象是否还存在，来决定是否调用事件处理函数
        std::weak_ptr<void> _tie;
        // 弱智能指针引用，可以通过tie，让它指向外部的对象
        // tie.lock()可以获取到外部对象的智能指针，
        // 如果获取成功，说明外部对象还存在，可以安全地调用事件处理函数；如果获取失败，说明外部对象已经被销毁了，就不应该调用事件处理函数了

        bool _addedToloop;   // 标志位：判断channel是否在poller监控中
        bool _eventHandling; // 标志位：判断channel是否正在事件处理中

        ReadCallBack _readCallback;
        EventCallBack _writeCallback;
        EventCallBack _errorCallback;
        EventCallBack _closeCallback;
    };
}