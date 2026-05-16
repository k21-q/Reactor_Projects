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

namespace net{
    class Channel;
    class Timestamp;
    //muduo库中将描述符的监控1+管理分开了
     // 移除事件监控： 只 epoll_ctl解除监控，但不移除管理
     // 移除监控管理： 即解除监控，也移除管理

    //抽象类，主要描述要实现的功能，具体实现在派生类中
    class Poller{ 
        public:
            virtual Timestamp wait(std::vector<Channel*> &actives)=0; //获取到所有的活跃就绪事件，通过就绪的描述符，找到channel
            // 要实现新增 以及 修改事件
            virtual void updateChannel(Channel* channel)=0;  //获取到描述符，然后获取到要监控的事件，来改变他
            //解除并移除监控
            virtual void removeChannel(Channel* channel)=0;  

        private:
            //map 红黑树 和unordered_map 哈希表 不一样
            std::unordered_map<int,Channel*> channels;  //文件描述符和Channel有一个映射关系，这样就能够找到Channel了

    };
}