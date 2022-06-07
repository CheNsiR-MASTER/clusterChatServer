#include "chatServer.h"
#include "chatService.h"

#include <functional>
#include <iostream>

using json = nlohmann::json;

ChatServer::ChatServer(
        muduo::net::EventLoop* loop,
        const muduo::net::InetAddress& listenAddr,
        const std::string& nameArg
    ):
    server_(loop, listenAddr,nameArg), loop_(loop)
{
    // 注册回调函数
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));

    // 注册读写回调函数
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置线程数量
    server_.setThreadNum(4);
}

void ChatServer::start()
{
    server_.start();
}

void ChatServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if(!conn->connected())
    {
        std::cout << "关闭连接 .... " << std::endl;
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
            muduo::net::Buffer* buffer,
            muduo::Timestamp time
        )
{
    std::string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    
    std::cout << js << std::endl;
    auto service = ChatService::instance();
    auto handler = service->getHandler(js["msgid"].get<int>());
    handler(conn, js, time);
}