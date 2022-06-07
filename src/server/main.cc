#include "chatServer.h"
#include "chatService.h"
#include <iostream>
#include <signal.h>

void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        std::cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << std::endl;
        exit(0);
    }

    char* ip = argv[1];
    int16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler);
    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr(ip, port);

    ChatServer server(&loop, listenAddr, "chatServer");
    
    server.start();
    loop.loop();
    return 0;
}