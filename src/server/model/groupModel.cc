#include "groupModel.h"
#include "db/db.h"

// 创建群组
bool GroupModel::createGroup(Group& group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str()
        );
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
        group.setId(mysql_insert_id(mysql.getConnection()));
        return true;
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, std::string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d, %d, '%s')", groupid, userid, role.c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
std::vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    /**
     * @brief 首先根据userid在groupuser表中查询出该用户所属的群组信息
     *        根据群组信息查询所有属于该群组的userid  => 和 user表联合查询，查出用户的详细信息
     */
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
         groupuser b on a.id = b.groupid where b.userid=%d",
            userid);

    std::vector<Group> groupVec;

    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);

                groupVec.emplace_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for(Group& group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a \
            inner join groupuser b on b.userid = a.id where b.groupid=%d",
                group.getId());
        
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setName(row[1]);
                groupuser.setState(row[2]);
                groupuser.setRole(row[3]);

                group.getUsers().emplace_back(groupuser);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 根据指定的groupid 查询群组用户id列表， 除了userid本身
    // => 主要用于群聊业务，给群组其他成员发消息
std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    std::vector<int> idVec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.emplace_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}