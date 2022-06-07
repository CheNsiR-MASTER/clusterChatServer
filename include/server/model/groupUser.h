#pragma once

#include "user.h"

// 群组用户， 多了一个role 角色信息，从user类直接继承复用 user的其他信息
class GroupUser : public User
{
public:
    void setRole(std::string role) { this->role = role; }
    std::string getRole() { return this->role; }
private:
    std::string role;
};