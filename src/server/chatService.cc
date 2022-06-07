#include "chatService.h"
#include "public.h"
#include "user.h"

#include <iostream>
#include <vector>

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = msgHandlerMap_.find(msgid);
    if (it == msgHandlerMap_.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const muduo::net::TcpConnectionPtr &conn, json &js, muduo::Timestamp) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

ChatService::ChatService()
{
    msgHandlerMap_.insert({LOGIN_MSG, std::bind(
        &ChatService::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({LOGINOUT_MSG, std::bind(
        &ChatService::loginout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({REG_MSG, std::bind(
        &ChatService::regist, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({ONE_CHAT_MSG, std::bind(
        &ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({ADD_FRIEND_MSG, std::bind(
        &ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({CREATE_GROUP_MSG, std::bind(
        &ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({ADD_GROUP_MSG, std::bind(
        &ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    msgHandlerMap_.insert({GROUP_CHAT_MSG, std::bind(
        &ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});

    // 连接redis服务器
    if(redis_.connect())
    {
        // 设置上报消息的回调
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMsg, this, std::placeholders::_1, std::placeholders::_2));
    }
}

// 处理登陆业务
void ChatService::login(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    std::string pwd =js["password"];
    User user = userModel_.query(id);
    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "account is logging";
            conn->send(response.dump());
        }
        else
        {
            // 临界区减小锁的粒度
            {
                std::lock_guard<std::mutex> lock(mtx_);
                connMap_.insert({user.getId(), conn});
            }

            // update state
            user.setState("online");
            userModel_.updateState(user);

            // login success
            // id用户登录成功后，向redis订阅channel(id)
            redis_.subscribe(id); 

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = id;
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            std::vector<std::string> vec = offlineModel_.query(user.getId());
            if(!vec.empty())
            {
                response["offlinemsg"] = vec;
                offlineModel_.remove(user.getId());
            }

            // 查询该用户的好友信息列表并返回
            std::vector<User> userVec = friendModel_.query(user.getId());
            if(!userVec.empty())
            {
                std::vector<std::string> vec2;
                for(auto& user : userVec)
                {
                    json friendResponse;
                    friendResponse["id"] = user.getId();
                    friendResponse["name"] = user.getName();
                    friendResponse["state"] = user.getState();

                    vec2.emplace_back(friendResponse.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            std::vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                std::vector<std::string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    std::vector<std::string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // login failed
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "username or password error";
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = connMap_.find(userid);
        if (it != connMap_.end())
        {
            connMap_.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    redis_.unsubcribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

// 处理注册业务
void ChatService::regist(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    
    bool flag = userModel_.insert(user);
    if(flag)
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();

        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 客户端的异常处理逻辑，找到conn 对应的 user，将用户state 更改为 offline
void ChatService::clientCloseException(const muduo::net::TcpConnectionPtr& conn)
{
    User user;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto it = connMap_.begin(); it != connMap_.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                connMap_.erase(it);
                break;
            }
        }
    }

    // 异常注销，属于下线
    redis_.unsubcribe(user.getId());

    if(user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

void ChatService::oneChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int toid = js["to"].get<int>();

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if(connMap_.count(toid))
        {
            // toid online
            connMap_[toid]->send(js.dump());
            return;
        }
    }

    // 查询 toid 是否在线(登录在其他服务器上)
    User user = userModel_.query(toid);
    if(user.getState() == "online")
    {
        redis_.publish(toid, js.dump());
        return;
    }

    // toid 不在线，存储离线消息
    offlineModel_.insert(toid, js.dump());
}

void ChatService::reset()
{
    userModel_.resetState();
}

void ChatService::addFriend(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    friendModel_.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if(groupModel_.createGroup(group))
    {
        groupModel_.addGroup(userid, group.getId(), "creator");
    }
}

// 添加群组业务
void ChatService::addGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int userid = js["userid"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> userIdVec = groupModel_.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(mtx_);
    for(int id : userIdVec)
    {
        if(connMap_.count(id))
        {
            connMap_[id]->send(js.dump());
        }
        else
        {
            // 查询 toid 是否在线(登录在其他服务器上)
            User user = userModel_.query(id);
            if(user.getState() == "online")
            {
                redis_.publish(id, js.dump());
            }
            else offlineModel_.insert(id, js.dump());
        }
    }
}

// 处理订阅通道中发送消息的回调
void ChatService::handleRedisSubscribeMsg(int userid, std::string msg)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = connMap_.find(userid);
    if(it != connMap_.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储离线消息
    offlineModel_.insert(userid, msg);
}