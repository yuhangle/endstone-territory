//
// Created by yuhang on 2025/9/3.
//

#ifndef TERRITORY_TERRITORY_ACTION_H
#define TERRITORY_TERRITORY_ACTION_H

#include <vector>
#include <optional>
#include "database.hpp"
#include "territory_instance.h"
#include "territory_types.h"

using namespace std;
class Territory;

class Territory_Action {
public:
    explicit Territory_Action(DataBase& database, Territory* territory_);

    // 定义三维坐标类型别名
    using Point3D = std::tuple<double, double, double>;
    //定义立方体类别
    using Cube = std::tuple<Point3D, Point3D>;

    //定义快速创建领地数据
    struct QuickTtyData {
        std::string player_name;
        std::string tty_type;
        Point3D pos1;
        std::string dim1;
        Point3D pos2;
        std::string dim2;
    };

    [[nodiscard]] std::map<std::string, TerritoryData>& get_all_tty() const;
    static const std::map<std::string, TerritoryData>& getAllTty();
    std::pair<bool, std::string> create_territory(const std::string& player_name, const Point3D &pos1, const Point3D &pos2, const Point3D &tppos, const std::string& dim);
    std::pair<bool, std::string> create_sub_territory(const std::string& player_name, const Point3D &pos1, const Point3D &pos2, const Point3D &tppos, const std::string& dim);
    static TerritoryData* read_territory_by_name(const std::string& territory_name);
    static bool isPointInCube(const tuple<double, double, double>& point,const tuple<double, double, double>& corner1,const tuple<double, double, double>& corner2);
    static bool isPointInCubeEdge(const tuple<double, double, double>& point,const tuple<double, double, double>& corner1,const tuple<double, double, double>& corner2);
    static bool is_overlapping(const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube1,const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube2);
    static bool isTerritoryOverlapping(const std::tuple<double, double, double>& new_pos1,const std::tuple<double, double, double>& new_pos2,const std::string& new_dim, bool resize = false, const std::string& tty_name = "");
    static bool check_in_tty_edge(const string& tty_name, const Point3D& point);
    static int check_tty_num(const std::string& player_name);
    static int get_tty_area(int x1, int z1, int x2, int z2);
    static std::tuple<double, double, double> pos_to_tuple(const std::string& str);
    static bool isSubsetCube(const Cube &cube1, const Cube &cube2);
    static std::pair<bool, std::string> listTrueFatherTTY(const std::string& playerName,const Cube& childCube,const std::string& childDim);
    static std::vector<TerritoryData> list_player_tty(const std::string& player_name);
    [[nodiscard]] std::vector<std::string> getSubTty(const std::string& tty_name) const;
    [[nodiscard]] bool del_Tty_by_name(const std::string& territory_name) const;
    [[nodiscard]] bool del_player_tty(const std::string &tty_name) const;
    [[nodiscard]] bool rename_Tty(const std::string& territory_name,const std::string& new_tty_name) const;
    [[nodiscard]] std::pair<bool, std::string> rename_player_tty(const std::string &oldname, const std::string &newname) const;
    static std::optional<bool> is_tty_owner(const std::string &ttyname, const std::string &player_name);
    static std::optional<bool> is_tty_op(const std::string &ttyname, const std::string &player_name);
    static std::optional<std::vector<InTtyInfo>> list_in_tty(const Point3D &pos, const std::string &dim);
    [[nodiscard]] bool change_tty_permissions(const std::string &ttyname,const std::string &permission,int value) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_permissions(const std::string &ttyname,const std::string &permission,int value) const;
    [[nodiscard]] int change_tty_member(const std::string &ttyname,const std::string &action,const std::string &player_name) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_member(const std::string &ttyname,const std::string &action,const std::string &player_name) const;
    [[nodiscard]] int change_tty_owner(const std::string &ttyname,const std::string &old_owner_name,const std::string &new_owner_name) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_owner(const std::string &ttyname,const std::string &old_owner_name,const std::string &new_owner_name) const;
    [[nodiscard]] int change_tty_manager(const std::string &ttyname,const std::string &action, const std::string &player_name) const;
    [[nodiscard]] std::pair<bool, std::string> change_territory_manager(const std::string &ttyname,const std::string &action,const std::string &player_name) const;
    static std::string pointToString(const Point3D &p);
    [[nodiscard]] std::pair<bool, std::string> change_tty_tppos(const std::string &ttyname,const Point3D &tppos,const std::string &dim) const;
    static std::vector<std::string> getPlayerTtyNames(const std::string& player_name);
    static std::vector<std::string> getMemberTtyNames(const std::string& player_name);
    static std::vector<std::string> getAllTtyNames();
    static std::vector<TerritoryData> getOpTtyList(const std::string& player_name);
    static std::vector<TerritoryData>getPlayerTtyList(const std::string& player_name);
    [[nodiscard]] std::pair<bool,std::string> resize_territory(const Point3D& pos1, const Point3D& pos2, const TerritoryData& old_tty_data, const Point3D& tppos) const;
    static void clearCache();
    std::shared_ptr<TerritoryInstance> getInstance(const std::string& name);
    static std::map<std::string, TerritoryData>& getAllTtyMutable();


private:
    Territory* territory_;
    DataBase& database_;
    inline static std::map<std::string, TerritoryData> all_tty;
};

#endif //TERRITORY_TERRITORY_ACTION_H