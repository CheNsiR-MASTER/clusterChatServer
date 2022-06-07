#pragma once

#include "user.h"

class UserModel
{
public:
    bool insert(User& user);

    // 根据 id 查询 user
    User query(int id);

    // 更新用户的状态信息
    bool updateState(User user);

    void resetState();
};