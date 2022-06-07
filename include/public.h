#pragma once

enum EnMsgType
{
    LOGIN_MSG = 1, // 登入的消息
    LOGIN_MSG_ACK, // 登入的响应消息
    LOGINOUT_MSG,
    REG_MSG,       // 注册的消息
    REG_MSG_ACK,   // 注册的响应消息
    ONE_CHAT_MSG,  // 聊天消息
    ADD_FRIEND_MSG, // 添加好友的消息
    CREATE_GROUP_MSG, // 创建群组消息
    ADD_GROUP_MSG,  // 加入群组消息
    GROUP_CHAT_MSG, // 群聊天
};