#pragma once
/*
  抽象事件处理器模块： 对描述符进行事件处理描述管理
  设计：
    1.要监控的描述符
    2.要监控的事件
    3.描述符事件就绪的事件
    4.针对时间如何处理的回调函数
*/
//事件处理器模块
#include<stddef.h>
#include<iostream>
#include<functional>


//11111
//12

//事件回调函数
namespace neet{
    class Timestamp;
    using ReadCallBack=std::function<void(Timestamp)>;  //读事件回调函数
    using EventCallBack=std::function<void()>;                //其他事件回调函数
    enum ChannelState{
        //描述符添加监控，有不同的状态，创建时没有状态，添加时才有
        KNew=1,  //新建还未添加事件监控
        KAdded,  //已添加事件监控
        KDeleted  //已解除事件监控
    };
    class Channel{
         public:
         int fd();
         uint32_t events();         //获取要监控的事件
         void setRevents(uint32_t e); //设置就绪事件
         void handleEvent();  //总的事件处理函数,任何就绪的事件就会调用这个函数  Poller类文件描述符监控后  会根据实际就绪的事件调用不同的类进行处理

         private:
            int _fd; //要监控的事件
            ChannelState _state; //事件状态
            uint32_t  _events; //要监控的事件 //EPOLLIN:
            uint32_t  _revents; //保存实际就绪的事件
            ReadCallBack _readcallback;
            EventCallBack _writecallback;
            EventCallBack _errorcallback;
            EventCallBack _closecallback;
    };
}