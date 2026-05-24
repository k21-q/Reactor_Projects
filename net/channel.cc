#include"eventloop.h"
#include"channel.h"
#include"log.h"

namespace net{


    Channel::Channel(int fd, EventLoop* loop)
     :_loop(loop)
     ,_fd(fd)
     ,_state(KNew)
     ,_events(0)
     ,_tied(false)
     ,_addedToloop(false)
     ,_eventHandling(false){
        LOG_DEBUG("construct Channel: %p",this);

     }
    Channel::~Channel(){
        LOG_DEBUG("~desstruct Channel: %p",this);
    }

    //解除监控，并移除管理
    void Channel::remove(){
        _loop->removeChannel(this);
    }

      //用于实际的调用poller，对描述符进行事件监控
    void Channel::update(){
        _loop->updateChannel(this);
    }
 
}