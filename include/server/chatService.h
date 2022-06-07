#pragma once

#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.h"
#include "userModel.h"
#include "offlineMsgModel.h"
#include "friendModel.h"
#include "groupModel.h"
#include "redis.h"

using json = nlohmann::json;

using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp)>;

// 将service 类设计为单例
class ChatService
{
public:
    static ChatService* instance();

    // 处理登陆业务
    void login(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);

    // 处理登出业务
    void loginout(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    
    // 处理注册业务
    void regist(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    void clientCloseException(const muduo::net::TcpConnectionPtr& conn);

    // 一对一聊天服务
    void oneChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);

    // 添加好友业务
    void addFriend(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time); 
    
    // 创建群组业务
    void createGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    
    // 添加群组业务
    void addGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    
    // 群组聊天业务
    void groupChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);

    // 重置user state 处理 ctrl + c 中断的异常
    void reset();

    // 处理订阅通道中发生的消息
    void handleRedisSubscribeMsg(int userid, std::string msg);
private:
    ChatService();
    UserModel userModel_;
    OfflineMsgModel offlineModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 封装 tcpConnectionPtr的map
    std::unordered_map<int, muduo::net::TcpConnectionPtr> connMap_;

    Redis redis_;
    std::mutex mtx_;
};