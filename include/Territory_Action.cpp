//
// Created by yuhang on 2025/9/3.
//

#include "Territory_Action.h"

#include "TerritoryCore.h"
Territory_Action::Territory_Action(DataBase database) : Database(std::move(database)) {}

// 从外部获取all_tty数据
const std::map<std::string, Territory_Action::TerritoryData>&
Territory_Action::getAllTty() {
    return all_tty;
}

//从数据库读取全部领地数据并载入全局变量all_tty
std::map<std::string, Territory_Action::TerritoryData>& Territory_Action::get_all_tty() const{
    vector<map<string,string>> result;
    Database.getAllTty(result);
    if (result.empty()) {
        Territory_Action::TerritoryData emptyData = {""};
        std::map<std::string, Territory_Action::TerritoryData> emptyDatas;
        emptyDatas.insert(make_pair("", emptyData));
        all_tty = emptyDatas;
        return all_tty;
    }
    all_tty.clear();
    std::map<std::string, Territory_Action::TerritoryData> new_all_tty;
    for (const auto& data : result) {
        TerritoryData tty_data;
        tty_data.name = data.at("name");
        tty_data.pos1 = {DataBase::stringToInt(data.at("pos1_x")),DataBase::stringToInt(data.at("pos1_y")),DataBase::stringToInt(data.at("pos1_z"))};
        tty_data.pos2 = {DataBase::stringToInt(data.at("pos2_x")),DataBase::stringToInt(data.at("pos2_y")),DataBase::stringToInt(data.at("pos2_z"))};
        tty_data.tppos = {DataBase::stringToInt(data.at("tppos_x")),DataBase::stringToInt(data.at("tppos_y")),DataBase::stringToInt(data.at("tppos_z"))};
        tty_data.owner = data.at("owner");
        tty_data.manager = data.at("manager");
        tty_data.member = data.at("member");
        tty_data.if_jiaohu = DataBase::stringToInt(data.at("if_jiaohu"));
        tty_data.if_break = DataBase::stringToInt(data.at("if_break"));
        tty_data.if_tp = DataBase::stringToInt(data.at("if_tp"));
        tty_data.if_build = DataBase::stringToInt(data.at("if_build"));
        tty_data.if_bomb = DataBase::stringToInt(data.at("if_bomb"));
        tty_data.if_damage = DataBase::stringToInt(data.at("if_damage"));
        tty_data.dim = data.at("dim");
        tty_data.father_tty = data.at("father_tty");
        new_all_tty[tty_data.name] = tty_data;
    }
    all_tty = new_all_tty;
    return all_tty;
};

// 根据名字读取领地信息的函数
Territory_Action::TerritoryData* Territory_Action::read_territory_by_name(const std::string& territory_name) {
    auto it = all_tty.find(territory_name);
    if (it != all_tty.end()) {
        return &it->second;
    }
    return nullptr; // 未找到，返回 nullptr
}

// 检查点是否在立方体范围内
bool Territory_Action::isPointInCube(const tuple<double, double, double>& point,
                          const tuple<double, double, double>& corner1,
                          const tuple<double, double, double>& corner2) {
    double x_min = min(get<0>(corner1), get<0>(corner2));
    double x_max = max(get<0>(corner1), get<0>(corner2));
    double y_min = min(get<1>(corner1), get<1>(corner2));
    double y_max = max(get<1>(corner1), get<1>(corner2));
    double z_min = min(get<2>(corner1), get<2>(corner2));
    double z_max = max(get<2>(corner1), get<2>(corner2));

    return (x_min <= get<0>(point) && get<0>(point) <= x_max &&
            y_min <= get<1>(point) && get<1>(point) <= y_max &&
            z_min <= get<2>(point) && get<2>(point) <= z_max);
}

// 检查立方体重合
bool Territory_Action::is_overlapping(const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube1,
                    const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube2) {
    // 对于每个cube，计算各轴的实际范围（不依赖于输入顺序）
    auto computeMinMax = [](const std::tuple<double, double, double>& point1, const std::tuple<double, double, double>& point2) {
        return std::make_tuple(
                std::min(std::get<0>(point1), std::get<0>(point2)),
                std::max(std::get<0>(point1), std::get<0>(point2)),
                std::min(std::get<1>(point1), std::get<1>(point2)),
                std::max(std::get<1>(point1), std::get<1>(point2)),
                std::min(std::get<2>(point1), std::get<2>(point2)),
                std::max(std::get<2>(point1), std::get<2>(point2))
        );
    };

    // 计算cube1的范围
    auto range1 = computeMinMax(cube1.first, cube1.second);
    double x1_min = std::get<0>(range1), x1_max = std::get<1>(range1);
    double y1_min = std::get<2>(range1), y1_max = std::get<3>(range1);
    double z1_min = std::get<4>(range1), z1_max = std::get<5>(range1);

    // 计算cube2的范围
    auto range2 = computeMinMax(cube2.first, cube2.second);
    double x2_min = std::get<0>(range2), x2_max = std::get<1>(range2);
    double y2_min = std::get<2>(range2), y2_max = std::get<3>(range2);
    double z2_min = std::get<4>(range2), z2_max = std::get<5>(range2);

    // 检查是否有维度不重叠
    bool overlap_x = x1_max >= x2_min && x2_max >= x1_min;
    bool overlap_y = y1_max >= y2_min && y2_max >= y1_min;
    bool overlap_z = z1_max >= z2_min && z2_max >= z1_min;

    return overlap_x && overlap_y && overlap_z;
}

// 检查领地重合
bool Territory_Action::isTerritoryOverlapping(const std::tuple<double, double, double>& new_pos1,
                            const std::tuple<double, double, double>& new_pos2,
                            const std::string& new_dim) {
    // 构造表示新领地范围的 pair
    auto new_tty = std::make_pair(new_pos1, new_pos2);

    return ranges::any_of(std::as_const(all_tty),
                          [&](const auto& entry) {
                              const TerritoryData& data = entry.second;
                              const auto& existing_pos1 = data.pos1;
                              const auto& existing_pos2 = data.pos2;
                              const std::string& existing_dim = data.dim;

                              return is_overlapping(new_tty, std::make_pair(existing_pos1, existing_pos2)) &&
                                     (new_dim == existing_dim);
                          });
}

// 检查玩家拥有领地数量的函数
int Territory_Action::check_tty_num(const std::string& player_name) {
    // 初始化计数器
    int tty_num = 0;
    for (const auto &val: all_tty | views::values) {
        const Territory_Action::TerritoryData& territory = val;
        if (player_name == territory.owner) {
            // 找到一个就加1
            tty_num++;
        }
    }
    return tty_num;
}

//计算面积
int Territory_Action::get_tty_area(const int x1,const int z1,const int x2,const int z2) {
    // 计算矩形的宽度和高度
    int width = abs(x2 - x1);
    int height = abs(z2 - z1);

    // 计算面积
    int area = width * height;

    // 返回面积
    return area;
}

// 用于将命令提取的pos转换成需要的值
std::tuple<double, double, double> Territory_Action::pos_to_tuple(const std::string& str) {
    std::tuple<double, double, double> result;
    std::stringstream ss(str);
    double value1, value2, value3;

    // 使用流提取操作符来解析字符串中的浮点数
    if (ss >> value1 >> value2 >> value3) {
        result = std::make_tuple(value1, value2, value3);
    }
    return result;
}

//检查立方体是否为子集函数
bool Territory_Action::isSubsetCube(const Territory_Action::Cube &cube1, const Territory_Action::Cube &cube2) {
    // 获取立方体 1 的最小和最大坐标
    auto [cube1_min_x, cube1_max_x] = std::minmax({get<0>(std::get<0>(cube1)), get<0>(std::get<1>(cube1))});
    auto [cube1_min_y, cube1_max_y] = std::minmax({get<1>(std::get<0>(cube1)), get<1>(std::get<1>(cube1))});
    auto [cube1_min_z, cube1_max_z] = std::minmax({get<2>(std::get<0>(cube1)), get<2>(std::get<1>(cube1))});

    // 获取立方体 2 的最小和最大坐标
    auto [cube2_min_x, cube2_max_x] = std::minmax({get<0>(std::get<0>(cube2)), get<0>(std::get<1>(cube2))});
    auto [cube2_min_y, cube2_max_y] = std::minmax({get<1>(std::get<0>(cube2)), get<1>(std::get<1>(cube2))});
    auto [cube2_min_z, cube2_max_z] = std::minmax({get<2>(std::get<0>(cube2)), get<2>(std::get<1>(cube2))});

    // 判断 cube1 是否在 cube2 的范围内
    return cube1_min_x >= cube2_min_x && cube1_max_x <= cube2_max_x &&
           cube1_min_y >= cube2_min_y && cube1_max_y <= cube2_max_y &&
           cube1_min_z >= cube2_min_z && cube1_max_z <= cube2_max_z;
}

//列出符合条件父领地的函数
std::pair<bool, std::string> Territory_Action::listTrueFatherTTY(const std::string& playerName,
                                               const Territory_Action::Cube& childCube,
                                               const std::string& childDim) {
    std::vector<std::string> trueTTYInfo;

    for (const auto &val: all_tty | views::values) {
        const Territory_Action::TerritoryData& ttyInfo = val;
        std::string ttyOwner = ttyInfo.owner;
        std::string ttyManager = ttyInfo.manager;
        Territory_Action::Point3D pos1 = ttyInfo.pos1;
        Territory_Action::Point3D pos2 = ttyInfo.pos2;
        std::string dim = ttyInfo.dim;
        std::string ttyName = ttyInfo.name;

        // 玩家为领地主或领地管理员
        if (playerName == ttyOwner || ttyManager.find(playerName) != std::string::npos) {
            // 领地维度相同
            if (childDim == dim) {
                // 检查全包围领地
                if (Territory_Action::isSubsetCube(childCube, std::make_tuple(pos1, pos2))) {
                    trueTTYInfo.push_back(ttyName);
                }
            }
        }
    }

    if (trueTTYInfo.size() > 1) {
        return {false, LangTty.getLocal("无法在子领地内创建子领地")};
    } else if (trueTTYInfo.empty()) {
        return {false, LangTty.getLocal("未查找到符合条件的父领地, 子领地创建失败")};
    } else if (trueTTYInfo.size() == 1) {
        return {true, trueTTYInfo.front()};
    } else {
        return {false, LangTty.getLocal("未知错误")};
    }
}

// 列出玩家领地的函数
std::vector<Territory_Action::TerritoryData> Territory_Action::list_player_tty(const std::string& player_name) {
    std::vector<TerritoryData> player_all_tty;
    for (const auto &territory: all_tty | views::values) {
        if (territory.owner == player_name) {
            player_all_tty.push_back(territory);
        }
    }
    return player_all_tty;  // 如果没有找到，返回的 vector 为空
}

//列出父领地的所有子领地名
std::vector<std::string> Territory_Action::getSubTty(const std::string &tty_name) const {
    std::vector<std::map<std::string, std::string>> result;
    Database.getTtyByFather(tty_name,result);
    if (result.empty()) {
        return {};
    }
    vector<std::string> sub_tty_name;
    sub_tty_name.reserve(result.size());
for (const auto &data: result) {
        sub_tty_name.push_back(data.at("name"));
    }
    return sub_tty_name;
}


// 删除领地函数
bool Territory_Action::del_Tty_by_name(const std::string& territory_name) const {
    auto tty_data = read_territory_by_name(territory_name);
    if (!tty_data) {
        return false;
    }
    // 成功找到领地，执行删除
    (void)Database.deleteTty(tty_data->name);

    // 获取子领地并修改它们的父领地
    auto sub_ttys = getSubTty(territory_name);
    for (const auto &sub_tty: sub_ttys) {
        (void)Database.updateValue("territories","father_tty","","name",sub_tty);
    }

    // 在所有数据库操作完成后，统一刷新全局领地缓存
    all_tty = get_all_tty();

    return true;
}

//重命名领地
bool Territory_Action::rename_Tty(const std::string &territory_name, const std::string &new_tty_name) const {
    if (read_territory_by_name(new_tty_name)) {
        return false;
    }
    (void)Database.updateValue("territories","name",new_tty_name,"name",territory_name);
    auto sub_ttys = getSubTty(territory_name);
    if (sub_ttys.empty()) {
        return true;
    }
    for (const auto &sub_tty: sub_ttys) {
        (void)Database.updateValue("territories","father_tty",new_tty_name,"name",sub_tty);
    }
    return true;
}

// 重命名玩家领地函数
[[nodiscard]] std::pair<bool, std::string> Territory_Action::rename_player_tty(const std::string &oldname, const std::string &newname) const{
    if (rename_Tty(oldname, newname)) {
        std::string msg = LangTty.getLocal("已重命名领地: 从 ") + oldname + LangTty.getLocal(" 到 ") + newname;
        (void)get_all_tty();
        return {true, msg};
    } else {
        std::string msg = LangTty.getLocal("尝试重命名领地但未找到: ") + oldname;
        return {false, msg};
    }
}

// 检查领地主人函数
std::optional<bool> Territory_Action::check_tty_owner(const std::string &ttyname, const std::string &player_name) {
    // 遍历全局缓存中的所有领地数据
    for (const auto &territory: all_tty | views::values) {
        if (territory.name == ttyname) {         // 找到对应领地
            if (territory.owner == player_name) {  // 领地主人匹配
                return true;
            } else {                             // 领地主人不匹配
                return false;
            }
        }
    }
    // 未找到该领地，返回空值
    return std::nullopt;
}

// 检查玩家是否为领地主人或管理员
std::optional<bool> Territory_Action::check_tty_op(const std::string &ttyname, const std::string &player_name) {
    // 遍历全局缓存中的每一条领地数据
    for (const auto &territory: all_tty | views::values) {
        if (territory.name == ttyname) {  // 找到对应领地
            // 拆分为管理员列表（以逗号分割）
            std::vector<std::string> currentManagers;
            if (!territory.manager.empty()) {
                currentManagers = DataBase::splitString(territory.manager);
            }
            if (player_name == territory.owner || ranges::find(currentManagers,player_name) != currentManagers.end())
                return true;
            else
                return false;
        }
    }
    // 未找到领地，返回空值
    return std::nullopt;
}

// 函数功能：列出与给定坐标点及维度匹配的全部领地信息
std::optional<std::vector<Territory_Action::InTtyInfo>> Territory_Action::list_in_tty(const Territory_Action::Point3D &pos, const std::string &dim) {
    /* 参数：
    //   pos: 点坐标
    //   dim: 点所在的维度
    // 返回：
    //   若找到至少一个匹配的领地，则返回包含各领地信息的 vector；
    //  若无匹配，则返回 std::nullopt。
    */
    std::vector<Territory_Action::InTtyInfo> in_tty;

    // 遍历所有领地数据
    for (const auto &val: all_tty | views::values) {
        const Territory_Action::TerritoryData &territory = val;
        // 维度匹配
        if (territory.dim == dim) {
            // 坐标匹配（调用 is_point_in_cube）
            if (Territory_Action::isPointInCube(pos, territory.pos1, territory.pos2)) {
                // 合并领地的所有成员：分别按逗号分割后拼接
                std::vector<std::string> combinedMembers;
                auto owners   = DataBase::splitString(territory.owner);
                auto managers = DataBase::splitString(territory.manager);
                auto members  = DataBase::splitString(territory.member);
                combinedMembers.insert(combinedMembers.end(), owners.begin(), owners.end());
                combinedMembers.insert(combinedMembers.end(), managers.begin(), managers.end());
                combinedMembers.insert(combinedMembers.end(), members.begin(), members.end());

                Territory_Action::InTtyInfo info{
                        territory.name,
                        territory.if_jiaohu,
                        territory.if_break,
                        territory.if_build,
                        territory.if_bomb,
                        territory.if_damage,
                        combinedMembers,
                        territory.owner
                };

                // 如果是子领地（father_tty 非空），则清空之前收集的非子领地信息，仅保留该记录，然后直接返回结果
                if (!territory.father_tty.empty()) {
                    in_tty.clear();
                    in_tty.push_back(info);
                    return in_tty;
                } else {
                    // 非子领地直接加入列表
                    in_tty.push_back(info);
                }
            }
        }
    }

    // 如果没有匹配的领地，则返回空（None风格的返回）
    if (in_tty.empty()) {
        return std::nullopt;
    } else {
        return in_tty;
    }
}

//领地权限更改
bool Territory_Action::change_tty_permissions(const std::string &ttyname, const std::string &permission, int value) const {
    if (const auto tty = read_territory_by_name(ttyname); tty == nullptr) {
        return false;
    }
    return Database.updateValue("territories",permission,std::to_string(value),"name",ttyname);
}

// 领地权限变更函数
std::pair<bool, std::string> Territory_Action::change_territory_permissions(const std::string &ttyname,const std::string &permission,int value) const{
    // 参数：
    //   ttyname: 领地名
    //   permission: 权限名（允许的选项为 "if_jiaohu", "if_break", "if_tp", "if_build", "if_bomb","if_damage"）
    //   value: 权限值
    // 返回：
    //   pair.first 为操作是否成功；pair.second 为相应提示信息

    // 定义允许变更的权限列表

    // 检查权限名是否合法
    if (std::vector<std::string> allowed_permissions = {"if_jiaohu", "if_break", "if_tp", "if_build", "if_bomb", "if_damage"}; ranges::find(allowed_permissions, permission) ==
                                                                                                                               allowed_permissions.end()) {
        std::string msg = LangTty.getLocal("无效的权限名: ") + permission;
        return {false, msg};
                                                                                                                               }

    if (change_tty_permissions(ttyname, permission, value)) {
        std::string msg = LangTty.getLocal("已更新领地 ") + ttyname + LangTty.getLocal(" 的权限 ") + permission + LangTty.getLocal(" 为 ") + std::to_string(value);
        // 更新全局领地信息
        (void)get_all_tty();
        return {true, msg};
    } else {
        std::string msg = LangTty.getLocal("尝试更新领地权限但未找到领地: ") + ttyname;
        return {false, msg};
    }
}

//添加或删除领地成员的函数
int Territory_Action::change_tty_member(const std::string &ttyname, const std::string &action, const std::string &player_name) const {
    auto tty_data = read_territory_by_name(ttyname);
    if (tty_data == nullptr) {
        return -1;
    }
    auto tty_members = DataBase::splitString(tty_data->member);
    if (action == "add") {
        if (ranges::find(tty_members, player_name) != tty_members.end()) {
            return 1;
        } else {
            tty_members.push_back(player_name);
        }
    } else {
        if (ranges::find(tty_members, player_name) == tty_members.end()) {
            return 1;
        } else {
            tty_members.erase(tty_members.end());
        }
    }
    auto new_tty_members = DataBase::vectorToString(tty_members);
    if (Database.updateValue("territories","member",new_tty_members,"name",ttyname)) {
        return 0;
    } else {
        return -2;
    }
}

// 添加或删除领地成员的函数
[[nodiscard]] std::pair<bool, std::string> Territory_Action::change_territory_member(const std::string &ttyname,
                                               const std::string &action,
                                               const std::string &player_name) const{
    // 参数：
    //   ttyname: 领地名称
    //   action: 操作类型 ("add" 或 "remove")
    //   player_name: 需要添加或删除的玩家名
    // 返回：
    //   pair.first 为操作是否成功；pair.second 为提示信息

    // 检查操作类型是否合法
    if (action != "add" && action != "remove") {
        return {false, "无效的操作类型: " + action};
    }
    auto status = change_tty_member(ttyname, action, player_name);
    if (status == -1) {
        std::string msg = LangTty.getLocal("尝试更改领地成员但未找到领地: ") + ttyname;
        return {false, msg};
    }

    std::string msg;

    if (action == "add") {
        // 添加操作：若不存在则添加
        if (status == 0) {
            msg = LangTty.getLocal("已添加成员到领地: ") + player_name + " -> " + ttyname;
        } else {
            msg = LangTty.getLocal("无需变更成员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
            return {true, msg};  // 状态未改变，视为成功
        }
    } else if (action == "remove") {
        // 删除操作：若存在则去除
        if (status == 0) {
            msg = LangTty.getLocal("已从领地中移除成员: ") + player_name + " <- " + ttyname;
        } else {
            msg = LangTty.getLocal("无需变更成员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
            return {true, msg};  // 状态未改变，视为成功
        }
    }

    if (status == 0) {
        // 更新全局领地信息
        (void)get_all_tty();
        return {true, msg};
    } else {
        return {false, LangTty.getLocal("更新领地成员时发生未知错误: ") + ttyname};
    }
}

//更改领地主人函数
int Territory_Action::change_tty_owner(const std::string &ttyname,const std::string &old_owner_name,const std::string &new_owner_name) const {
    auto tty_data = read_territory_by_name(ttyname);
    if (tty_data == nullptr) {
        return -1;
    }
    auto tty_owner = tty_data->owner;
    if (tty_owner != old_owner_name || tty_owner == new_owner_name) {
        return -2;
    }
    if (Database.updateValue("territories","owner",new_owner_name,"name",ttyname)) {
        return 0;
    } else {
        return 1;
    }
}

// 领地转让函数
[[nodiscard]] std::pair<bool, std::string> Territory_Action::change_territory_owner(const std::string &ttyname,
                                              const std::string &old_owner_name,
                                              const std::string &new_owner_name) const{
    // 参数：
    //   ttyname:        领地名称
    //   old_owner_name: 原主人玩家名
    //   new_owner_name: 新主人玩家名
    // 返回：
    //   std::pair<bool, std::string>，first 为是否成功，second 为提示信息

    // 1. 检查新领地主人的领地数量是否达到上限
    if (Territory_Action::check_tty_num(new_owner_name) >= max_tty_num) {
        std::string msg = LangTty.getLocal("玩家 ") + new_owner_name + LangTty.getLocal(" 的领地数量已达到上限, 无法增加新的领地, 转让领地失败");
        return {false, msg};
    }
    auto status = change_tty_owner(ttyname,old_owner_name,new_owner_name);
    if (status == -1) {
        std::string msg = LangTty.getLocal("尝试更改领地主人但未找到领地: ") + ttyname;
        return {false, msg};
    }
    // 判断当前领地主人是否匹配，并且新旧领主不能相同
    if (status == 0) {
        // 准备更新为新主人
        std::string msg = LangTty.getLocal("领地转让成功, 领地主人已由 ") + old_owner_name + LangTty.getLocal(" 变更为 ") + new_owner_name;

        // 更新全局领地信息
        (void)get_all_tty();
        return {true, msg};
    } else if (status == 1) {
        return {false,"Update data failed"};
    }
    else {
        std::string msg = LangTty.getLocal("领地转让失败, 请检查是否为领地主人以及转让对象是否合规");
        return {false, msg};
    }
}

//添加或删除领地管理员的函数
int Territory_Action::change_tty_manager(const std::string &ttyname, const std::string &action, const std::string &player_name) const {
    auto tty_data = read_territory_by_name(ttyname);
    if (tty_data == nullptr) {
        return -1;
    }
    auto tty_manager = DataBase::splitString(tty_data->manager);
    if (action == "add") {
        if (ranges::find(tty_manager, player_name) != tty_manager.end()) {
            return 1;
        } else {
            tty_manager.push_back(player_name);
        }
    } else {
        if (ranges::find(tty_manager, player_name) == tty_manager.end()) {
            return 1;
        } else {
            tty_manager.erase(tty_manager.end());
        }
    }
    auto new_tty_managers = DataBase::vectorToString(tty_manager);
    if (Database.updateValue("territories","manager",new_tty_managers,"name",ttyname)) {
        return 0;
    } else {
        return -2;
    }
}

// 添加或删除领地管理员的函数
[[nodiscard]] std::pair<bool, std::string> Territory_Action::change_territory_manager(const std::string &ttyname,
                                               const std::string &action,
                                               const std::string &player_name) const {
    // 参数：
    //   ttyname: 领地名称
    //   action: 操作类型 ("add" 或 "remove")
    //   player_name: 需要添加或删除的玩家名
    // 返回：
    //   pair.first 为操作是否成功；pair.second 为提示信息

    // 检查操作类型是否合法
    if (action != "add" && action != "remove") {
        return {false, "无效的操作类型: " + action};
    }
    auto status = change_tty_manager(ttyname, action, player_name);
    if (status == -1) {
        std::string msg = LangTty.getLocal("尝试更改领地管理员但未找到领地: ") + ttyname;
        return {false, msg};
    }

    std::string msg;

    if (action == "add") {
        // 添加操作：若不存在则添加
        if (status == 0) {
            msg = LangTty.getLocal("已添加管理员到领地: ") + player_name + " -> " + ttyname;
        } else {
            msg = LangTty.getLocal("无需变更管理员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
            return {true, msg};  // 状态未改变，视为成功
        }
    } else if (action == "remove") {
        // 删除操作：若存在则去除
        if (status == 0) {
            msg = LangTty.getLocal("已从领地中移除管理员: ") + player_name + " <- " + ttyname;
        } else {
            msg = LangTty.getLocal("无需变更管理员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
            return {true, msg};  // 状态未改变，视为成功
        }
    }

    if (status == 0) {
        // 更新全局领地信息
        (void)get_all_tty();
        return {true, msg};
    } else {
        return {false, LangTty.getLocal("更新领地管理员时发生未知错误: ") + ttyname};
    }
}

// 帮助将Point转换为字符串（用于提示信息）
std::string Territory_Action::pointToString(const Point3D &p) {
    return "(" + std::to_string(std::get<0>(p)) + ","
           + std::to_string(std::get<1>(p)) + ","
           + std::to_string(std::get<2>(p)) + ")";
}

// 领地传送点变更函数
[[nodiscard]] std::pair<bool, std::string> Territory_Action::change_tty_tppos(const std::string &ttyname,
                                              const Territory_Action::Point3D &tppos,
                                              const std::string &dim) const{
    // 参数：
    //   ttyname: 领地名
    //   tppos:   新的传送点坐标（Point类型）
    //   dim:     当前所在维度
    // 返回：
    //   std::pair<bool, std::string>，first为操作是否成功，second为提示信息


    // 在更新数据库前，先检查传送点是否合法
    // 遍历全局缓存中的所有领地记录
    for (const auto &val: all_tty | views::values) {
        const Territory_Action::TerritoryData &territory = val;
        if (territory.name == ttyname) {
            // 维度检查
            if (territory.dim != dim) {
                std::string msg = LangTty.getLocal("你当前所在的维度(") + dim + LangTty.getLocal(")与领地维度(") + territory.dim + LangTty.getLocal(")不匹配, 无法设置领地传送点");
                return {false, msg};
            }
            // 坐标检查
            if (!Territory_Action::isPointInCube(tppos, territory.pos1, territory.pos2)) {
                std::string pos_str = Territory_Action::pointToString(tppos);
                std::string msg = LangTty.getLocal("无法接受的坐标") + pos_str + LangTty.getLocal(", 领地传送点不能位于领地之外!");
                return {false, msg};
            }
            // 检查通过则退出循环
            break;
        }
    }

    // 执行更新操作：更新数据库中该领地的传送点坐标
    sqlite3* db = nullptr;
    int rc = sqlite3_open(db_file.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "无法打开数据库:" << errmsg << std::endl;
        sqlite3_close(db);
        return {false, "无法打开数据库: " + errmsg};
    }

    // 构造SQL更新语句
    std::string update_query = "UPDATE territories SET tppos_x=?, tppos_y=?, tppos_z=? WHERE name=?";
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, update_query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "准备更新语句失败:" << errmsg << std::endl;
        sqlite3_close(db);
        return {false, "准备更新语句失败: " + errmsg};
    }

    // 绑定传送点坐标参数
    double x = std::get<0>(tppos);
    double y = std::get<1>(tppos);
    double z = std::get<2>(tppos);
    rc = sqlite3_bind_double(stmt, 1, x);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "绑定 tppos_x 参数失败:" << errmsg << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return {false, "绑定 tppos_x 参数失败: " + errmsg};
    }
    rc = sqlite3_bind_double(stmt, 2, y);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "绑定 tppos_y 参数失败:" << errmsg << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return {false, "绑定 tppos_y 参数失败: " + errmsg};
    }
    rc = sqlite3_bind_double(stmt, 3, z);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "绑定 tppos_z 参数失败:" << errmsg << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return {false, "绑定 tppos_z 参数失败: " + errmsg};
    }
    // 绑定领地名称
    rc = sqlite3_bind_text(stmt, 4, ttyname.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "绑定领地名失败:" << errmsg << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return {false, "绑定领地名失败: " + errmsg};
    }

    // 执行更新语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string errmsg = sqlite3_errmsg(db);
        std::cerr << "更新领地传送点失败:" << errmsg << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return {false, "更新领地传送点失败: " + errmsg};
    }
    int affectedRows = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (affectedRows > 0) {
        std::string pos_str = Territory_Action::pointToString(tppos);
        std::string msg = LangTty.getLocal("已更新领地 ") + ttyname + LangTty.getLocal(" 的传送点为 ") + pos_str;
        // 更新全局领地信息缓存
        (void)get_all_tty();
        return {true, msg};
    } else {
        std::string msg = LangTty.getLocal("尝试更新领地传送点但未找到领地: ") + ttyname;
        return {false, msg};
    }
}