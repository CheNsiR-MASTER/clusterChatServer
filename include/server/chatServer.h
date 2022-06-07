#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <string>

#include "json.h"

class ChatServer
{
public:
    // 初始化 server
    ChatServer(
        muduo::net::EventLoop* loop,
        const muduo::net::InetAddress& listenAddr,
        const std::string& nameArg
    );
    
    void start();

private:
    muduo::net::TcpServer server_;
    muduo::net::EventLoop* loop_;

    // 上报链接相关信息的回调函数
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                muduo::net::Buffer* buffer,
                muduo::Timestamp time
    );
};