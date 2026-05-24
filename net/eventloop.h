#pragma once
//二次通道的循环模块

#include<memory>



namespace net{
    class Channel;
    class Poller;
    class EventLoop{
        public:
            void updateChannel(Channel* channel);  //  获取到描述符，然后获取到要监控的事件，来改变他
            //解除并移除监控   并移除管理
            void removeChannel(Channel* channel);  
        private:
           std::unique_ptr<Poller> _poller;  //事件循环中使用的IO复用器
     
    };
}