//
// Created by yuhang on 2025/9/3.
//

#ifndef TERRITORY_TERRITORY_ACTION_H
#define TERRITORY_TERRITORY_ACTION_H

#include <vector>
#include <optional>
#include "DataBase.h"
using namespace std;

class Territory_Action {
public:
    explicit Territory_Action(DataBase database);

    //定义领地数据结构
    struct TerritoryData {
        std::string name;
        std::tuple<double, double, double> pos1 = {0,0,0};
        std::tuple<double, double, double> pos2 = {0,0,0};
        std::tuple<double, double, double> tppos = {0,0,0};
        std::string owner;
        std::string manager;
        std::string member;
        bool if_jiaohu = false;
        bool if_break = false;
        bool if_tp = false;
        bool if_build = false;
        bool if_bomb = false;
        bool if_damage = false;
        std::string dim;
        std::string father_tty;
    };
    // 定义三维坐标类型别名
    using Point3D = std::tuple<double, double, double>;
    //定义立方体类别
    using Cube = std::tuple<Point3D, Point3D>;
    /*
    //定义领地内玩家数据结构
    struct TtyEveryone {
        std::string owner;
        std::string manager;
        std::string member;
    };
     */
    // 定义领地事件检测返回信息结构（按顺序对应：领地名、交互、破坏、放置、爆炸权限，合并后的成员列表，领地主）
    struct InTtyInfo {
        std::string name;
        bool if_jiaohu;
        bool if_break;
        bool if_build;
        bool if_bomb;
        bool if_damage;
        std::vector<std::string> members;
        std::string owner;
    };

    [[nodiscard]] int get_all_tty() const;
    static const std::map<std::string, TerritoryData>& getAllTty();
    static TerritoryData* read_territory_by_name(const std::string& territory_name);
    static bool isPointInCube(const tuple<double, double, double>& point,const tuple<double, double, double>& corner1,const tuple<double, double, double>& corner2);
    static bool is_overlapping(const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube1,const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube2);
    static bool isTerritoryOverlapping(const std::tuple<double, double, double>& new_pos1,const std::tuple<double, double, double>& new_pos2,const std::string& new_dim);
    static int check_tty_num(const std::string& player_name);
    static int get_tty_area(int x1, int z1, int x2, int z2);
    static std::tuple<double, double, double> pos_to_tuple(const std::string& str);
    static bool isSubsetCube(const Cube &cube1, const Cube &cube2);
    static std::pair<bool, std::string> listTrueFatherTTY(const std::string& playerName,const Cube& childCube,const std::string& childDim);
    static std::vector<TerritoryData> list_player_tty(const std::string& player_name);
    std::vector<std::string> getSubTty(const std::string& tty_name) const;
    bool del_Tty_by_name(const std::string& territory_name) const;
    bool rename_Tty(const std::string& territory_name,const std::string& new_tty_name) const;
    [[nodiscard]] std::pair<bool, std::string> rename_player_tty(const std::string &oldname, const std::string &newname) const;
    static std::optional<bool> check_tty_owner(const std::string &ttyname, const std::string &player_name);
    static std::optional<bool> check_tty_op(const std::string &ttyname, const std::string &player_name);
    static std::optional<std::vector<InTtyInfo>> list_in_tty(const Point3D &pos, const std::string &dim);
    bool change_tty_permissions(const std::string &ttyname,const std::string &permission,int value) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_permissions(const std::string &ttyname,const std::string &permission,int value) const;
    int change_tty_member(const std::string &ttyname,const std::string &action,const std::string &player_name) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_member(const std::string &ttyname,const std::string &action,const std::string &player_name) const;
    int change_tty_owner(const std::string &ttyname,const std::string &old_owner_name,const std::string &new_owner_name) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_owner(const std::string &ttyname,const std::string &old_owner_name,const std::string &new_owner_name) const;
    int change_tty_manager(const std::string &ttyname,const std::string &action, const std::string &player_name) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_manager(const std::string &ttyname,const std::string &action,const std::string &player_name) const;
    static std::string pointToString(const Point3D &p);
    [[nodiscard]] std::pair<bool, std::string> change_tty_tppos(const std::string &ttyname,const Territory_Action::Point3D &tppos,const std::string &dim) const;


private:
    DataBase Database;
    static std::map<std::string, TerritoryData> all_tty;
};

#endif //TERRITORY_TERRITORY_ACTION_H