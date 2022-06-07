#include "redis.h"
#include <iostream>

Redis::Redis():
    _publish_context(nullptr), _subcribe_context(nullptr)
{

}

Redis::~Redis()
{
    if(_publish_context) redisFree(_publish_context);
    if(_subcribe_context) redisFree(_subcribe_context);
}

// 连接redis服务器
bool Redis::connect()
{
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(nullptr == _publish_context)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if(nullptr == _subcribe_context)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息的话给业务层上报
    std::thread t([&] (){
        observer_channel_message();
    });
    t.detach();
    std::cout << "connect redis-server success!" << std::endl;
    return true;
}

// 向redis指定的通道，publish消息
bool Redis::publish(int channel, std::string msg)
{
    redisReply* reply = (redisReply*) redisCommand(_publish_context, 
        "PUBLISH %d %s", channel, msg.c_str());
    if(nullptr == reply)
    {
        std::cerr <<  "publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道，subcribe消息
bool Redis::subscribe(int channel)
{
    // subscribe 命令会导致线程阻塞，这里只完成订阅，不做接受消息的处理
    // 接受消息的处理在 子线程的 observer_channel_handler中处理
    if(REDIS_ERR == redisAppendCommand(_subcribe_context, "SUBSCRIBE %d", channel))
    {
        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(_subcribe_context, &done))
        {
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}

// 向redis指定的通道，unsubcribe消息
bool Redis::unsubcribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(_subcribe_context, "UNSUBSCRIBE %d", channel))
    {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }

    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(_subcribe_context, &done))
        {
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中，接受订阅通道的消息
void Redis::observer_channel_message()
{
    redisReply* reply = nullptr;
    while(REDIS_OK == redisGetReply(_subcribe_context, (void **)reply))
    {
        if(reply != nullptr &&
            reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str),
                reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<" << std::endl;
}

// 向业务层通报消息通道的回调对象
void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->_notify_message_handler = fn;
}
