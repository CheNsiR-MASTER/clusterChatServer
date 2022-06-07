#include "hiredis/hiredis.h"
#include <thread>
#include <functional>

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的通道，publish消息
    bool publish(int channel, std::string msg);

    // 向redis指定的通道，subcribe消息
    bool subscribe(int channel);

    // 向redis指定的通道，unsubcribe消息
    bool unsubcribe(int channel);

    // 在独立线程中，接受订阅通道的消息
    void observer_channel_message();

    // 向业务层通报消息通道的回调对象
    void init_notify_handler(std::function<void(int, std::string)> fn);

private:
    // redis上下文信息，负责处理public消息
    redisContext* _publish_context;
    // redis上下文信息，负责处理subcribe消息
    redisContext* _subcribe_context;
    // 回调操作，收到订阅层消息，向service提交
    std::function<void(int, std::string)> _notify_message_handler;
};