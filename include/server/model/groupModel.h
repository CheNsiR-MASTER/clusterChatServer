#pragma once

#include "group.h"
#include <string>
#include <vector>

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group& group);

    // 加入群组
    void addGroup(int userid, int groupid, std::string role);

    // 查询用户所在群组信息
    std::vector<Group> queryGroups(int userid);

    // 根据指定的groupid 查询群组用户id列表， 除了userid本身
        // => 主要用于群聊业务，给群组其他成员发消息
    std::vector<int> queryGroupUsers(int userid, int groupid);
};