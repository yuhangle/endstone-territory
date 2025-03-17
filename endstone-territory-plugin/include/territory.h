//
// Created by yuhang on 2025/3/6.
//
#include <endstone/endstone.hpp>
#include <endstone/plugin/plugin.h>
#include <endstone/color_format.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <sqlite3.h>
#include <map>
#include <tuple>
#include <utility>
#include <algorithm>
#include <endstone/player.h>
#include <endstone/server.h>
#include <sstream>
#include <vector>
#include <endstone/form/form.h>
#include <endstone/form/action_form.h>
#include <optional>
#include <endstone/level/position.h>
#include <endstone/actor/actor.h>
#include <endstone/event/block/block_break_event.h>
#include <endstone/event/block/block_place_event.h>
#include <endstone/event/actor/actor_explode_event.h>
#include <endstone/event/player/player_interact_event.h>
#include <endstone/event/player/player_interact_actor_event.h>
#include <endstone/event/actor/actor_damage_event.h>
#include <endstone/command/command_sender_wrapper.h>
#include "translate.h"

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;
translate_tty LangTty;

//数据文件路径
std::string data_path = "plugins/territory";
std::string config_path = "plugins/territory/config.json";
const std::string db_file = "plugins/territory/territory_data.db";

int max_tty_num;
bool actor_fire_attack_protect;

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
//定义领地数据缓存
std::map<std::string, TerritoryData> all_tty;

// 定义三维坐标类型别名
using Point3D = std::tuple<double, double, double>;
//定义立方体类别
using Cube = std::tuple<Point3D, Point3D>;
//全局玩家位置信息
std::unordered_map<std::string, std::tuple<Point3D, string, string>> lastPlayerPositions;
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

class Territory : public endstone::Plugin {


public:

    // 读取配置文件
    json read_config() {
        std::ifstream i(config_path);
        json j;
        try {
            i >> j;
            return j;
        } catch (json::parse_error& ex) { // 捕获解析错误
            getLogger().error( ex.what());
            json error_value = {
                    {"error","error"}
            };
            return error_value;
        }
    }

    //数据目录和配置文件检查
    void datafile_check() {
        json df_config = {
                {"player_max_tty_num", 20},
                {"actor_fire_attack_protect", true}
        };

        if (!(std::filesystem::exists(data_path))) {
            getLogger().info(LangTty.getLocal("Territory数据目录未找到,已自动创建"));
            std::filesystem::create_directory("plugins/territory");
            if (!(std::filesystem::exists("plugins/territory/config.json"))) {
                std::ofstream file(config_path);
                if (file.is_open()) {
                    file << df_config.dump(4);
                    file.close();
                    getLogger().info(LangTty.getLocal("配置文件已创建"));
                }
            }
        } else if (std::filesystem::exists(data_path)) {
            if (!(std::filesystem::exists("plugins/territory/config.json"))) {
                std::ofstream file(config_path);
                if (file.is_open()) {
                    file << df_config.dump(4);
                    file.close();
                    getLogger().info(LangTty.getLocal("配置文件已创建"));
                }
            } else {
                bool need_update = false;
                json loaded_config;

                // 加载现有配置文件
                std::ifstream file(config_path);
                file >> loaded_config;

                // 检查配置完整性并更新
                for (auto& [key, value] : df_config.items()) {
                    if (!loaded_config.contains(key)) {
                        loaded_config[key] = value;
                        getLogger().info(LangTty.getLocal("配置项 '{}' 已根据默认配置进行更新"), key);
                        need_update = true;
                    }
                }

                // 如果需要更新配置文件，则进行写入
                if (need_update) {
                    std::ofstream outfile(config_path);
                    if (outfile.is_open()) {
                        outfile << loaded_config.dump(4);
                        outfile.close();
                        getLogger().info(LangTty.getLocal("配置文件已完成更新"));
                    }
                }
            }
        }
    }

// 初始化 SQLite 数据库
    int initDatabase() {
        sqlite3* db;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc) {
            getLogger().error(LangTty.getLocal("无法打开数据库: "),sqlite3_errmsg(db));
            return rc;
        }
        const char* sql = "CREATE TABLE IF NOT EXISTS territories ("
                          "name TEXT PRIMARY KEY, "
                          "pos1_x REAL, pos1_y REAL, pos1_z REAL, "
                          "pos2_x REAL, pos2_y REAL, pos2_z REAL, "
                          "tppos_x REAL, tppos_y REAL, tppos_z REAL, "
                          "owner TEXT, manager TEXT, member TEXT, "
                          "if_jiaohu INTEGER, if_break INTEGER, if_tp INTEGER, "
                          "if_build INTEGER, if_bomb INTEGER, if_damage INTEGER, dim TEXT, father_tty TEXT);";
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            getLogger().error("创建表失败",errMsg);
            sqlite3_free(errMsg);
            sqlite3_close(db);
            return rc;
        }
        //0.2.0更新数据库
        // 检查并重新构建表结构以插入 if_damage 到 if_bomb 之后
        std::string checkSql = "PRAGMA table_info(territories);";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, checkSql.c_str(), -1, &stmt, nullptr);
        bool hasIfDamage = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* columnName = sqlite3_column_text(stmt, 1);
            if (strcmp((const char*)columnName, "if_damage") == 0) {
                hasIfDamage = true;
                break;
            }
        }
        sqlite3_finalize(stmt);

        if (!hasIfDamage) {
            // 如果没有 if_damage 字段，重新创建表结构
            char* errMsg = nullptr;

            // 1. 创建一个新的表，具有期望的列顺序
            std::string createTempTableSql = "CREATE TABLE territories_new ("
                                             "name TEXT PRIMARY KEY, "
                                             "pos1_x REAL, pos1_y REAL, pos1_z REAL, "
                                             "pos2_x REAL, pos2_y REAL, pos2_z REAL, "
                                             "tppos_x REAL, tppos_y REAL, tppos_z REAL, "
                                             "owner TEXT, manager TEXT, member TEXT, "
                                             "if_jiaohu INTEGER, if_break INTEGER, if_tp INTEGER, "
                                             "if_build INTEGER, if_bomb INTEGER, if_damage INTEGER DEFAULT 0, "
                                             "dim TEXT, father_tty TEXT);";
            rc = sqlite3_exec(db, createTempTableSql.c_str(), 0, 0, &errMsg);
            if (rc != SQLITE_OK) {
                getLogger().error("创建新表失败", errMsg);
                sqlite3_free(errMsg);
                sqlite3_close(db);
                return rc;
            }

            // 2. 将旧表的数据插入到新表中
            std::string copyDataSql = "INSERT INTO territories_new ("
                                      "name, pos1_x, pos1_y, pos1_z, "
                                      "pos2_x, pos2_y, pos2_z, "
                                      "tppos_x, tppos_y, tppos_z, "
                                      "owner, manager, member, "
                                      "if_jiaohu, if_break, if_tp, "
                                      "if_build, if_bomb, dim, father_tty) "
                                      "SELECT "
                                      "name, pos1_x, pos1_y, pos1_z, "
                                      "pos2_x, pos2_y, pos2_z, "
                                      "tppos_x, tppos_y, tppos_z, "
                                      "owner, manager, member, "
                                      "if_jiaohu, if_break, if_tp, "
                                      "if_build, if_bomb, dim, father_tty "
                                      "FROM territories;";
            rc = sqlite3_exec(db, copyDataSql.c_str(), 0, 0, &errMsg);
            if (rc != SQLITE_OK) {
                getLogger().error("复制数据到新表失败", errMsg);
                sqlite3_free(errMsg);
                sqlite3_close(db);
                return rc;
            }

            // 3. 删除旧表
            std::string dropOldTableSql = "DROP TABLE territories;";
            rc = sqlite3_exec(db, dropOldTableSql.c_str(), 0, 0, &errMsg);
            if (rc != SQLITE_OK) {
                getLogger().error("删除旧表失败", errMsg);
                sqlite3_free(errMsg);
                sqlite3_close(db);
                return rc;
            }

            // 4. 将新表重命名为旧表
            std::string renameTableSql = "ALTER TABLE territories_new RENAME TO territories;";
            rc = sqlite3_exec(db, renameTableSql.c_str(), 0, 0, &errMsg);
            if (rc != SQLITE_OK) {
                getLogger().error("重命名新表失败", errMsg);
                sqlite3_free(errMsg);
                sqlite3_close(db);
                return rc;
            } else {
                getLogger().info("成功插入 if_damage 字段到 if_bomb 之后");
            }
        }
        //关闭数据库
        sqlite3_close(db);
        return SQLITE_OK;
    }

// 向数据库写入数据
    int insertData(const std::string& name,
                   double pos1_x, double pos1_y, double pos1_z,
                   double pos2_x, double pos2_y, double pos2_z,
                   double tppos_x, double tppos_y, double tppos_z,
                   const std::string& owner,
                   const std::string& manager,
                   const std::string& member,
                   int if_jiaohu, int if_break, int if_tp,
                   int if_build, int if_bomb, int if_damage, const std::string& dim,
                   const std::string& father_tty) {
        sqlite3* db;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc) {
            getLogger().error("无法打开数据库: ",sqlite3_errmsg(db));
            return rc;
        }
        std::string sql = "INSERT INTO territories (name, pos1_x, pos1_y, pos1_z, "
                          "pos2_x, pos2_y, pos2_z, tppos_x, tppos_y, tppos_z, "
                          "owner, manager, member, if_jiaohu, if_break, if_tp, "
                          "if_build, if_bomb, if_damage, dim, father_tty) VALUES ('" + name + "', "
                          + std::to_string(pos1_x) + ", " + std::to_string(pos1_y) + ", " + std::to_string(pos1_z) + ", "
                          + std::to_string(pos2_x) + ", " + std::to_string(pos2_y) + ", " + std::to_string(pos2_z) + ", "
                          + std::to_string(tppos_x) + ", " + std::to_string(tppos_y) + ", " + std::to_string(tppos_z) + ", '"
                          + owner + "', '" + manager + "', '" + member + "', "
                          + std::to_string(if_jiaohu) + ", " + std::to_string(if_break) + ", " + std::to_string(if_tp) + ", "
                          + std::to_string(if_build) + ", " + std::to_string(if_bomb) + ", " + std::to_string(if_damage) + ", '" + dim + "', '"
                          + father_tty + "');";
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            getLogger().error(LangTty.getLocal("插入数据失败: "),errMsg);
            sqlite3_free(errMsg);
            sqlite3_close(db);
            return rc;
        }
        sqlite3_close(db);
        return SQLITE_OK;
    }

    // 从数据库读取所有领地数据
    int readAllTerritories() {
        sqlite3* db;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc) {
            getLogger().error("无法打开数据库: ",sqlite3_errmsg(db));
            return rc;
        }
        const char* sql = "SELECT name, pos1_x, pos1_y, pos1_z, pos2_x, pos2_y, pos2_z, tppos_x, tppos_y, tppos_z, owner, manager, member, if_jiaohu, if_break, if_tp, if_build, if_bomb, if_damage, dim, father_tty FROM territories;";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            getLogger().error("准备语句失败: ",sqlite3_errmsg(db));
            sqlite3_close(db);
            return rc;
        }
        all_tty.clear(); // 清空原有数据
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            TerritoryData data;
            data.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            data.pos1 = std::make_tuple(sqlite3_column_double(stmt, 1), sqlite3_column_double(stmt, 2), sqlite3_column_double(stmt, 3));
            data.pos2 = std::make_tuple(sqlite3_column_double(stmt, 4), sqlite3_column_double(stmt, 5), sqlite3_column_double(stmt, 6));
            data.tppos = std::make_tuple(sqlite3_column_double(stmt, 7), sqlite3_column_double(stmt, 8), sqlite3_column_double(stmt, 9));
            data.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
            data.manager = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            data.member = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            data.if_jiaohu = sqlite3_column_int(stmt, 13) != 0;
            data.if_break = sqlite3_column_int(stmt, 14) != 0;
            data.if_tp = sqlite3_column_int(stmt, 15) != 0;
            data.if_build = sqlite3_column_int(stmt, 16) != 0;
            data.if_bomb = sqlite3_column_int(stmt, 17) != 0;
            data.if_damage = sqlite3_column_int(stmt, 18) != 0;
            data.dim = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 19));
            data.father_tty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
            all_tty[data.name] = data;
        }
        if (rc != SQLITE_DONE) {
            getLogger().error(LangTty.getLocal("读取数据失败: "),sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return SQLITE_OK;
    }

    // 根据名字读取领地信息的函数
    TerritoryData* read_territory_by_name(const std::string& territory_name) {
        /**
         * 根据名字读取领地信息。
         *
         * 参数:
         * territory_name (std::string): 领地的名字。
         * 返回:
         * TerritoryData*: 如果找到对应的领地，则返回指向领地信息的指针；否则返回 nullptr。
         */
        auto it = all_tty.find(territory_name);
        if (it != all_tty.end()) {
            return &it->second; // 返回指向找到的 TerritoryData 的指针
        }
        return nullptr; // 未找到，返回 nullptr
    }

// 检查点是否在立方体范围内
    static bool isPointInCube(const tuple<double, double, double>& point,
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
    bool is_overlapping(const std::pair<std::tuple<double, double, double>, std::tuple<double, double, double>>& cube1,
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
    bool isTerritoryOverlapping(const std::tuple<double, double, double>& new_pos1,
                                const std::tuple<double, double, double>& new_pos2,
                                const std::string& new_dim) {
        // 构造表示新领地范围的 pair
        auto new_tty = std::make_pair(new_pos1, new_pos2);

        // 遍历所有已存在的领地数据
        for (const auto &entry: all_tty) {
            const TerritoryData &data = entry.second;
            // 取出已存领地的坐标和维度
            const auto &existing_pos1 = data.pos1;
            const auto &existing_pos2 = data.pos2;
            const std::string &existing_dim = data.dim;

            // 先判断在空间上是否重合
            if (is_overlapping(new_tty, std::make_pair(existing_pos1, existing_pos2))) {
                // 若重合，再检查维度是否相同
                if (new_dim == existing_dim) {
                    return true;
                }
            }
        }
        return false;
    }

    // 检查玩家拥有领地数量的函数
    int check_tty_num(const std::string& player_name) {
        // 初始化计数器
        int tty_num = 0;
        for (const auto& pair : all_tty) {
            const TerritoryData& territory = pair.second;
            if (player_name == territory.owner) {
                // 找到一个就加1
                tty_num++;
            }
        }
        return tty_num;
    }
//添加领地函数
    void player_add_tty(std::string player_name, Point3D pos1, Point3D pos2, Point3D tppos, std::string dim) {
        // 检查玩家领地数量是否达到上限
        if (check_tty_num(player_name) >= max_tty_num) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地"));
            return;
        }
        // 检查新领地是否与其他领地重叠
        if (isTerritoryOverlapping(pos1, pos2, dim)) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("此区域与其他玩家领地重叠"));
            return;
        }
        // 检查传送点是否在领地内
        if (!isPointInCube(tppos, pos1, pos2)) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("你当前所在的位置不在你要添加的领地上!禁止远程施法"));
            return;
        } else {
            // 使用当前时间作为领地名
            // 使用当前时间和父领地名作为新领地名
            auto now = std::chrono::system_clock::now();
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d%H%M%S");
            std::time_t current_time = std::chrono::system_clock::to_time_t(now);
            std::string name = ss.str();

            // 插入新领地数据到数据库
            insertData(name,
                       std::get<0>(pos1), std::get<1>(pos1), std::get<2>(pos1),
                       std::get<0>(pos2), std::get<1>(pos2), std::get<2>(pos2),
                       std::get<0>(tppos), std::get<1>(tppos), std::get<2>(tppos),
                       player_name, "", "", false, false, false, false, false,false, dim, "");

            // 向玩家发送成功消息
            getServer().getPlayer(player_name)->sendMessage(LangTty.getLocal("成功添加领地"));

            // 更新全局数据
            readAllTerritories();
        }
    }
// 用于将命令提取的pos转换成需要的值
    std::tuple<double, double, double> pos_to_tuple(const std::string& str) {
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
    bool isSubsetCube(const Cube &cube1, const Cube &cube2) {
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
    std::pair<bool, std::string> listTrueFatherTTY(const std::string& playerName,
                                                   const Cube& childCube,
                                                   const std::string& childDim) {
        std::vector<std::string> trueTTYInfo;

        for (const auto& item : all_tty) {
            const TerritoryData& ttyInfo = item.second;
            std::string ttyOwner = ttyInfo.owner;
            std::string ttyManager = ttyInfo.manager;
            Point3D pos1 = ttyInfo.pos1;
            Point3D pos2 = ttyInfo.pos2;
            std::string dim = ttyInfo.dim;
            std::string ttyName = ttyInfo.name;

            // 玩家为领地主或领地管理员
            if (playerName == ttyOwner || ttyManager.find(playerName) != std::string::npos) {
                // 领地维度相同
                if (childDim == dim) {
                    // 检查全包围领地
                    if (isSubsetCube(childCube, std::make_tuple(pos1, pos2))) {
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

    //用于添加玩家子领地的函数
    std::pair<bool, std::string> player_add_sub_tty(const std::string& playername,
                                                   const Point3D& pos1,
                                                   const Point3D& pos2,
                                                   const Point3D& tppos,
                                                   const std::string& dim) {
        // 检查玩家领地数量是否已达上限
        if (check_tty_num(playername) >= max_tty_num) {
            getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地"));
            return {false, ""};
        }

        // 检查传送点是否在领地内
        if (!isPointInCube(tppos, pos1, pos2)) {
            getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("你当前所在的位置不在你要添加的领地上!禁止远程施法"));
            return {false, ""};
        }

        // 父领地检查
        auto fatherTTYInfo = listTrueFatherTTY(playername, std::make_tuple(pos1, pos2), dim);
        if (!fatherTTYInfo.first) {
            getServer().getPlayer(playername)->sendErrorMessage(fatherTTYInfo.second);
            return {false, ""};
        } else {
            // 使用当前时间和父领地名作为新领地名
            auto now = std::chrono::system_clock::now();
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d%H%M%S");

            std::string childTTYName = fatherTTYInfo.second + "_" + ss.str();

            // 创建新领地数据结构
            TerritoryData newTTYData;
            newTTYData.name = childTTYName;
            newTTYData.pos1 = pos1;
            newTTYData.pos2 = pos2;
            newTTYData.tppos = tppos;
            newTTYData.owner = playername;
            newTTYData.dim = dim;
            newTTYData.father_tty = fatherTTYInfo.second;

            // 写入数据库
            insertData(childTTYName,
                           get<0>(pos1),get<1>(pos1),get<2>(pos1),
                           get<0>(pos2),get<1>(pos2),get<2>(pos2),
                           get<0>(tppos),get<1>(tppos),get<2>(tppos),playername,"","",
                           false,false,false,false,false,false,dim,fatherTTYInfo.second);
            getServer().getPlayer(playername)->sendMessage(LangTty.getLocal("成功添加子领地,归属于父领地") + fatherTTYInfo.second);
            // 更新全局数据
            readAllTerritories();
            return {true, childTTYName};
        }
    }

    // 提示领地信息函数
    void tips_online_players() {
        auto online_player_list = getServer().getOnlinePlayers();

        for (const auto &player : online_player_list) {
            string player_name = player->getName();
            string player_dim = player->getLocation().getDimension()->getName();
            Point3D player_pos = {player->getLocation().getBlockX(), player->getLocation().getBlockY(),
                                  player->getLocation().getBlockZ()};

            // 检查玩家位置是否有变化
            if (get<0>(lastPlayerPositions[player_name]) == player_pos) {
                // 玩家位置未改变
                continue;
            } else if (get<0>(lastPlayerPositions[player_name]) == tuple{0.000000, 0.000000, 0.000000}) {
                // 玩家未被录入全局位置信息，先录入当前位置，再等待下一次检测
                get<0>(lastPlayerPositions[player_name]) = player_pos;
                continue;
            }

            // 保存上一次的领地信息
            std::string previous_territory = get<1>(lastPlayerPositions[player_name]);
            std::string previous_father_territory = get<2>(lastPlayerPositions[player_name]); // 上次父领地

            // 先更新玩家位置
            get<0>(lastPlayerPositions[player_name]) = player_pos;

            // 遍历所有领地数据，确定玩家所在的最精细的领地（若存在子领地，则取子领地）
            const TerritoryData* selectedTerritory = nullptr;
            for (const auto &row : all_tty) {
                const TerritoryData &data = row.second;
                if (data.dim == player_dim && isPointInCube(player_pos, data.pos1, data.pos2)) {
                    // 第一次匹配到的领地先赋值
                    if (selectedTerritory == nullptr) {
                        selectedTerritory = &data;
                    } else {
                        // 如果当前遍历到的领地是子领地而已经选中的是父领地，则覆盖
                        // 前提：子领地的father_tty不为空，而父领地为空
                        if (!data.father_tty.empty() && selectedTerritory->father_tty.empty()) {
                            selectedTerritory = &data;
                        }
                        // 如果已有子领地，再判断是否需要更新
                    }
                }
            }

            // 根据检测结果发送消息（进入或切换领地，则发送欢迎词；退出所有领地则发送离开提示）
            if (selectedTerritory != nullptr) {
                // 当前在某个领地内
                std::string current_territory = selectedTerritory->name;
                std::string current_father_territory = selectedTerritory->father_tty; // 若为空则为父领地，否则为子领地

                // 如果领地名称改变，说明玩家进入新领地（包括从父领地到子领地的切换）
                if (previous_territory != current_territory) {
                    std::string msg;
                    if (!current_father_territory.empty()) {
                        // 进入子领地
                        msg = LangTty.getLocal("§2[领地] §r欢迎来到 ") + selectedTerritory->owner + LangTty.getLocal(" 的子领地 ") + current_territory;
                    } else {
                        // 进入普通领地
                        msg = LangTty.getLocal("§2[领地] §r欢迎来到 ") + selectedTerritory->owner + LangTty.getLocal(" 的领地 ") + current_territory;
                    }
                    auto pEntity = getServer().getPlayer(player_name);
                    if (pEntity != nullptr) {
                        //pEntity->sendToast("  ", msg);
                        //pEntity->sendMessage(msg);//聊天栏提示
                        pEntity->sendTip(msg);//tip提示
                    }
                    // 更新全局记录的领地信息
                    get<1>(lastPlayerPositions[player_name]) = current_territory;
                    get<2>(lastPlayerPositions[player_name]) = current_father_territory;
                }
            } else {
                // 玩家不在任何领地内，若之前记录了领地，则说明是离开领地行为，发送离开提示
                if (!previous_territory.empty()) {
                    std::string msg = LangTty.getLocal("§2[领地] §r您已离开领地 ") + previous_territory + LangTty.getLocal(", 欢迎下次再来");
                    auto pEntity = getServer().getPlayer(player_name);
                    if (pEntity != nullptr) {
                        //pEntity->sendToast("  ", msg);
                        //pEntity->sendMessage(msg);//聊天栏提示
                        pEntity->sendTip(msg);//tip提示
                    }
                    // 清空记录
                    get<1>(lastPlayerPositions[player_name]) = "";
                    get<2>(lastPlayerPositions[player_name]) = "";
                }
            }
        }
    }

    // 列出玩家领地的函数
    std::vector<TerritoryData> list_player_tty(const std::string& player_name) {
        std::vector<TerritoryData> player_all_tty;
        for (const auto& [key, territory] : all_tty) {
            if (territory.owner == player_name) {
                player_all_tty.push_back(territory);
            }
        }
        return player_all_tty;  // 如果没有找到，返回的 vector 为空
    }

    // 删除玩家领地函数，删除名称为 tty_name 的领地，并更新相关数据
    bool del_player_tty(const std::string &tty_name) {
        sqlite3* db = nullptr;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc != SQLITE_OK) {
            getLogger().error("无法打开数据库:", sqlite3_errmsg(db));
            return false;
        }

        // --- 1. 查找所有 father_tty 为 tty_name 的相关领地 ---
        const std::string selectSql = "SELECT name FROM territories WHERE father_tty = ?";
        sqlite3_stmt* selectStmt = nullptr;
        rc = sqlite3_prepare_v2(db, selectSql.c_str(), -1, &selectStmt, nullptr);
        if (rc != SQLITE_OK) {
            getLogger().error("准备查询语句失败:", sqlite3_errmsg(db));
            sqlite3_close(db);
            return false;
        }

        rc = sqlite3_bind_text(selectStmt, 1, tty_name.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            getLogger().error("绑定查询参数失败:", sqlite3_errmsg(db));
            sqlite3_finalize(selectStmt);
            sqlite3_close(db);
            return false;
        }

        // 收集所有相关领地的名称
        std::vector<std::string> related_territory_names;
        while ((rc = sqlite3_step(selectStmt)) == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(selectStmt, 0);
            if (text) {
                related_territory_names.push_back(reinterpret_cast<const char*>(text));
            }
        }
        if (rc != SQLITE_DONE) {
            getLogger().error("查询相关领地时出错:", sqlite3_errmsg(db));
            sqlite3_finalize(selectStmt);
            sqlite3_close(db);
            return false;
        }
        sqlite3_finalize(selectStmt);

        // --- 2. 如果存在相关领地，则更新其 father_tty 为空字符串 ---
        if (!related_territory_names.empty()) {
            const std::string updateSql = "UPDATE territories SET father_tty = ? WHERE name = ?";
            sqlite3_stmt* updateStmt = nullptr;
            rc = sqlite3_prepare_v2(db, updateSql.c_str(), -1, &updateStmt, nullptr);
            if (rc != SQLITE_OK) {
                getLogger().error("准备更新语句失败:", sqlite3_errmsg(db));
                sqlite3_close(db);
                return false;
            }
            for (const auto &related_name : related_territory_names) {
                sqlite3_reset(updateStmt);
                // 绑定空字符串作为更新参数
                rc = sqlite3_bind_text(updateStmt, 1, "", -1, SQLITE_TRANSIENT);
                if (rc != SQLITE_OK) {
                    getLogger().error("绑定空字符串失败:", sqlite3_errmsg(db));
                    sqlite3_finalize(updateStmt);
                    sqlite3_close(db);
                    return false;
                }
                // 绑定相关领地名称
                rc = sqlite3_bind_text(updateStmt, 2, related_name.c_str(), -1, SQLITE_TRANSIENT);
                if (rc != SQLITE_OK) {
                    getLogger().error("绑定领地名称失败:", sqlite3_errmsg(db));
                    sqlite3_finalize(updateStmt);
                    sqlite3_close(db);
                    return false;
                }

                rc = sqlite3_step(updateStmt);
                if (rc != SQLITE_DONE) {
                    getLogger().error("更新相关领地失败:", sqlite3_errmsg(db));
                    sqlite3_finalize(updateStmt);
                    sqlite3_close(db);
                    return false;
                }
            }
            sqlite3_finalize(updateStmt);
        }

        // --- 3. 执行删除操作 ---
        const std::string deleteSql = "DELETE FROM territories WHERE name = ?";
        sqlite3_stmt* deleteStmt = nullptr;
        rc = sqlite3_prepare_v2(db, deleteSql.c_str(), -1, &deleteStmt, nullptr);
        if (rc != SQLITE_OK) {
            getLogger().error("准备删除语句失败:", sqlite3_errmsg(db));
            sqlite3_close(db);
            return false;
        }
        rc = sqlite3_bind_text(deleteStmt, 1, tty_name.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            getLogger().error("绑定删除参数失败:", sqlite3_errmsg(db));
            sqlite3_finalize(deleteStmt);
            sqlite3_close(db);
            return false;
        }
        rc = sqlite3_step(deleteStmt);
        if (rc != SQLITE_DONE) {
            getLogger().error("删除领地失败:", sqlite3_errmsg(db));
            sqlite3_finalize(deleteStmt);
            sqlite3_close(db);
            return false;
        }

        // 获取删除操作影响的行数
        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(deleteStmt);
        sqlite3_close(db);

        // --- 4. 如果删除成功，则更新全局缓存 ---
        if (affectedRows > 0) {
            readAllTerritories();
            return true;
        } else {
            return false;
        }
    }

    // 重命名玩家领地函数
    std::pair<bool, std::string> rename_player_tty(const std::string &oldname, const std::string &newname) {
        sqlite3* db = nullptr;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("无法打开数据库:", errmsg);
            sqlite3_close(db);
            return {false, "无法打开数据库: " + errmsg};
        }

        // 1. 检查新名字是否已存在
        const char* selectCountSql = "SELECT COUNT(*) FROM territories WHERE name = ?";
        sqlite3_stmt* stmtCount = nullptr;
        rc = sqlite3_prepare_v2(db, selectCountSql, -1, &stmtCount, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备查询语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备查询语句失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtCount, 1, newname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定查询参数失败:", errmsg);
            sqlite3_finalize(stmtCount);
            sqlite3_close(db);
            return {false, "绑定查询参数失败: " + errmsg};
        }
        rc = sqlite3_step(stmtCount);
        if (rc != SQLITE_ROW) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("查询执行失败:", errmsg);
            sqlite3_finalize(stmtCount);
            sqlite3_close(db);
            return {false, "查询执行失败: " + errmsg};
        }
        int count = sqlite3_column_int(stmtCount, 0);
        sqlite3_finalize(stmtCount);
        if (count > 0) {
            std::string msg = LangTty.getLocal("无法重命名领地: 新名字 ") + newname + LangTty.getLocal(" 已存在");
            sqlite3_close(db);
            return {false, msg};
        }

        // 2. 执行重命名操作

        // 2.1 查询所有 father_tty 为旧名字的相关领地
        const char* selectSql = "SELECT name FROM territories WHERE father_tty = ?";
        sqlite3_stmt* stmtSelect = nullptr;
        rc = sqlite3_prepare_v2(db, selectSql, -1, &stmtSelect, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备查询相关领地语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备查询相关领地语句失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtSelect, 1, oldname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定查询参数失败:", errmsg);
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, "绑定查询参数失败: " + errmsg};
        }
        std::vector<std::string> related_names;
        while ((rc = sqlite3_step(stmtSelect)) == SQLITE_ROW) {
            const unsigned char* text = sqlite3_column_text(stmtSelect, 0);
            if (text) {
                related_names.push_back(reinterpret_cast<const char*>(text));
            }
        }
        if (rc != SQLITE_DONE) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("查询相关领地失败:", errmsg);
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, "查询相关领地失败: " + errmsg};
        }
        sqlite3_finalize(stmtSelect);

        // 2.2 如果存在相关领地，则更新其 father_tty 为 newname
        if (!related_names.empty()) {
            const char* updateFatherSql = "UPDATE territories SET father_tty = ? WHERE name = ?";
            sqlite3_stmt* stmtUpdateFather = nullptr;
            rc = sqlite3_prepare_v2(db, updateFatherSql, -1, &stmtUpdateFather, nullptr);
            if (rc != SQLITE_OK) {
                std::string errmsg = sqlite3_errmsg(db);
                getLogger().error("准备更新 father_tty 语句失败:", errmsg);
                sqlite3_close(db);
                return {false, "准备更新 father_tty 语句失败: " + errmsg};
            }
            for (const auto &relName : related_names) {
                sqlite3_reset(stmtUpdateFather);
                rc = sqlite3_bind_text(stmtUpdateFather, 1, newname.c_str(), -1, SQLITE_TRANSIENT);
                if (rc != SQLITE_OK) {
                    std::string errmsg = sqlite3_errmsg(db);
                    getLogger().error("绑定新名字失败:", errmsg);
                    sqlite3_finalize(stmtUpdateFather);
                    sqlite3_close(db);
                    return {false, "绑定新名字失败: " + errmsg};
                }
                rc = sqlite3_bind_text(stmtUpdateFather, 2, relName.c_str(), -1, SQLITE_TRANSIENT);
                if (rc != SQLITE_OK) {
                    std::string errmsg = sqlite3_errmsg(db);
                    getLogger().error("绑定领地名称失败:", errmsg);
                    sqlite3_finalize(stmtUpdateFather);
                    sqlite3_close(db);
                    return {false, "绑定领地名称失败: " + errmsg};
                }
                rc = sqlite3_step(stmtUpdateFather);
                if (rc != SQLITE_DONE) {
                    std::string errmsg = sqlite3_errmsg(db);
                    getLogger().error("更新相关领地 father_tty 失败:", errmsg);
                    sqlite3_finalize(stmtUpdateFather);
                    sqlite3_close(db);
                    return {false, "更新相关领地 father_tty 失败: " + errmsg};
                }
            }
            sqlite3_finalize(stmtUpdateFather);
        }

        // 2.3 执行重命名，将旧名字更新为新名字
        const char* updateNameSql = "UPDATE territories SET name = ? WHERE name = ?";
        sqlite3_stmt* stmtUpdateName = nullptr;
        rc = sqlite3_prepare_v2(db, updateNameSql, -1, &stmtUpdateName, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备重命名语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备重命名语句失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtUpdateName, 1, newname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定新名字失败:", errmsg);
            sqlite3_finalize(stmtUpdateName);
            sqlite3_close(db);
            return {false, "绑定新名字失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtUpdateName, 2, oldname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定旧名字失败:", errmsg);
            sqlite3_finalize(stmtUpdateName);
            sqlite3_close(db);
            return {false, "绑定旧名字失败: " + errmsg};
        }
        rc = sqlite3_step(stmtUpdateName);
        if (rc != SQLITE_DONE) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("重命名操作失败:", errmsg);
            sqlite3_finalize(stmtUpdateName);
            sqlite3_close(db);
            return {false, "重命名操作失败: " + errmsg};
        }
        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(stmtUpdateName);
        sqlite3_close(db);

        // 3. 返回结果并更新全局缓存
        if (affectedRows > 0) {
            std::string msg = LangTty.getLocal("已重命名领地: 从 ") + oldname + LangTty.getLocal(" 到 ") + newname;
            readAllTerritories();
            return {true, msg};
        } else {
            std::string msg = LangTty.getLocal("尝试重命名领地但未找到: ") + oldname;
            return {false, msg};
        }
    }

    // 检查领地主人函数
    std::optional<bool> check_tty_owner(const std::string &ttyname, const std::string &player_name) {
        // 遍历全局缓存中的所有领地数据
        for (const auto& [key, territory] : all_tty) {
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
    std::optional<bool> check_tty_op(const std::string &ttyname, const std::string &player_name) {
        // 遍历全局缓存中的每一条领地数据
        for (const auto &[key, territory]: all_tty) {
            if (territory.name == ttyname) {  // 找到对应领地
                if (player_name == territory.owner || player_name == territory.manager)
                    return true;
                else
                    return false;
            }
        }
        // 未找到领地，返回空值
        return std::nullopt;
    }
/* 本应用于菜单的函数,未调用
    // 列出领地的主人、管理员、成员信息的函数
    std::optional<TtyEveryone> list_tty_everyone(const std::string& ttyname) {
        // 遍历全局缓存中的每一个领地数据
        for (const auto &[key, territory]: all_tty) {
            if (territory.name == ttyname) {  // 找到匹配的领地
                TtyEveryone everyone{
                        territory.owner,
                        territory.manager,
                        territory.member
                };
                return everyone;
            }
        }
        // 未找到该领地，返回空值
        return std::nullopt;
    }

    // 列出所有玩家为领地的主人、管理员的领地的函数
    std::optional<std::vector<TerritoryData>> list_tty_op(const std::string &player_name) {
        std::vector<TerritoryData> op_tty_list;

        // 遍历全局缓存中的所有领地数据
        for (const auto &pair : all_tty) {
            const TerritoryData &territory = pair.second;
            // 检查玩家是否为领地主人或管理员
            if (player_name == territory.owner || player_name == territory.manager) {
                op_tty_list.push_back(territory);
            }
        }

        if (op_tty_list.empty()) {
            return std::nullopt;
        } else {
            return op_tty_list;
        }
    }

    // 列出所有玩家权限为成员及以上的领地的函数
    std::optional<std::vector<TerritoryData>> list_member_tty(const std::string &player_name) {
        std::vector<TerritoryData> member_tty_list;

        // 遍历全局缓存中的所有领地数据
        for (const auto & [key, territory] : all_tty) {
            if (player_name == territory.owner ||
                player_name == territory.manager ||
                player_name == territory.member) {
                member_tty_list.push_back(territory);
            }
        }

        if (member_tty_list.empty()) {
            return std::nullopt;
        } else {
            return member_tty_list;
        }
    }
*/

    // 辅助函数：按照指定分隔符分割字符串
    std::vector<std::string> split(const std::string &s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            // 可按需求去除前后的空格，这里简单处理
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    // 函数功能：列出与给定坐标点及维度匹配的全部领地信息
// 参数：
//   pos: 点坐标
//   dim: 点所在的维度
// 返回：
//   若找到至少一个匹配的领地，则返回包含各领地信息的 vector；
//   若无匹配，则返回 std::nullopt。
    std::optional<std::vector<InTtyInfo>> list_in_tty(const Point3D &pos, const std::string &dim) {
        std::vector<InTtyInfo> in_tty;

        // 遍历所有领地数据
        for (const auto &pair : all_tty) {
            const TerritoryData &territory = pair.second;
            // 维度匹配
            if (territory.dim == dim) {
                // 坐标匹配（调用 is_point_in_cube）
                if (isPointInCube(pos, territory.pos1, territory.pos2)) {
                    // 合并领地的所有成员：分别按逗号分割后拼接
                    std::vector<std::string> combinedMembers;
                    auto owners   = split(territory.owner, ',');
                    auto managers = split(territory.manager, ',');
                    auto members  = split(territory.member, ',');
                    combinedMembers.insert(combinedMembers.end(), owners.begin(), owners.end());
                    combinedMembers.insert(combinedMembers.end(), managers.begin(), managers.end());
                    combinedMembers.insert(combinedMembers.end(), members.begin(), members.end());

                    InTtyInfo info{
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

    // 领地权限变更函数
// 参数：
//   ttyname: 领地名
//   permission: 权限名（允许的选项为 "if_jiaohu", "if_break", "if_tp", "if_build", "if_bomb","if_damage"）
//   value: 权限值
// 返回：
//   pair.first 为操作是否成功；pair.second 为相应提示信息
    std::pair<bool, std::string> change_tty_permissions(const std::string &ttyname,
                                                        const std::string &permission,
                                                        int value) {
        // 定义允许变更的权限列表
        std::vector<std::string> allowed_permissions = {"if_jiaohu", "if_break", "if_tp", "if_build", "if_bomb", "if_damage"};

        // 检查权限名是否合法
        if (std::find(allowed_permissions.begin(), allowed_permissions.end(), permission) ==
            allowed_permissions.end()) {
            std::string msg = LangTty.getLocal("无效的权限名: ") + permission;
            return {false, msg};
        }

        sqlite3 *db = nullptr;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("无法打开数据库: ", errmsg);
            sqlite3_close(db);
            return {false, "无法打开数据库: " + errmsg};
        }

        // 构造更新 SQL 语句
        // 注意：使用拼接 permission 字符串时需确保 permission 合法，本例中已做检查
        std::string update_query = "UPDATE territories SET " + permission + "=? WHERE name=?";

        sqlite3_stmt *stmt = nullptr;
        rc = sqlite3_prepare_v2(db, update_query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备更新语句失败: ", errmsg);
            sqlite3_close(db);
            return {false, "准备更新语句失败: " + errmsg};
        }

        // 绑定参数：第一个参数为 value，第二个参数为 ttyname
        rc = sqlite3_bind_int(stmt, 1, value);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定权限值失败: ", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "绑定权限值失败: " + errmsg};
        }

        rc = sqlite3_bind_text(stmt, 2, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败: ", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }

        // 执行更新操作
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("更新领地权限失败: ", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "更新领地权限失败: " + errmsg};
        }

        // 获取受影响的行数
        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        if (affectedRows > 0) {
            std::string msg = LangTty.getLocal("已更新领地 ") + ttyname + LangTty.getLocal(" 的权限 ") + permission + LangTty.getLocal(" 为 ") + std::to_string(value);
            // 更新全局领地信息
            readAllTerritories();
            return {true, msg};
        } else {
            std::string msg = LangTty.getLocal("尝试更新领地权限但未找到领地: ") + ttyname;
            return {false, msg};
        }
    }

    // 辅助函数：将字符串向量用 delimiter 连接成一个字符串
    std::string join(const std::vector<std::string>& vec, char delimiter) {
        std::string result;
        for (size_t i = 0; i < vec.size(); ++i) {
            result += vec[i];
            if (i < vec.size() - 1) {
                result.push_back(delimiter);
            }
        }
        return result;
    }

// 添加或删除领地成员的函数
// 参数：
//   ttyname: 领地名称
//   action: 操作类型 ("add" 或 "remove")
//   player_name: 需要添加或删除的玩家名
// 返回：
//   pair.first 为操作是否成功；pair.second 为提示信息
    std::pair<bool, std::string> change_tty_member(const std::string &ttyname,
                                                   const std::string &action,
                                                   const std::string &player_name) {
        // 检查操作类型是否合法
        if (action != "add" && action != "remove") {
            return {false, "无效的操作类型: " + action};
        }

        sqlite3 *db = nullptr;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("无法打开数据库:", errmsg);
            sqlite3_close(db);
            return {false, "无法打开数据库: " + errmsg};
        }

        // 1. 查询当前成员列表
        std::string selectQuery = "SELECT member FROM territories WHERE name=?";
        sqlite3_stmt *stmtSelect = nullptr;
        rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmtSelect, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备查询语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备查询语句失败: " + errmsg};
        }

        rc = sqlite3_bind_text(stmtSelect, 1, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败:", errmsg);
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }

        rc = sqlite3_step(stmtSelect);
        if (rc != SQLITE_ROW) {
            std::string msg = LangTty.getLocal("尝试更改领地成员但未找到领地: ") + ttyname;
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, msg};
        }

        // 获取当前成员字符串（可能为空字符串）
        const unsigned char *text = sqlite3_column_text(stmtSelect, 0);
        std::string currentMembersStr = text ? reinterpret_cast<const char *>(text) : "";
        sqlite3_finalize(stmtSelect);

        // 分割成成员列表（以逗号为分隔符）
        std::vector<std::string> currentMembers;
        if (!currentMembersStr.empty()) {
            currentMembers = split(currentMembersStr, ',');
        }

        std::string msg;
        bool needUpdate = false;  // 是否需要执行数据库更新

        if (action == "add") {
            // 添加操作：若不存在则添加
            if (std::find(currentMembers.begin(), currentMembers.end(), player_name) == currentMembers.end()) {
                currentMembers.push_back(player_name);
                msg = LangTty.getLocal("已添加成员到领地: ") + player_name + " -> " + ttyname;
                needUpdate = true;
            } else {
                msg = LangTty.getLocal("无需变更成员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
                sqlite3_close(db);
                return {true, msg};  // 状态未改变，视为成功
            }
        } else if (action == "remove") {
            // 删除操作：若存在则去除
            auto it = std::find(currentMembers.begin(), currentMembers.end(), player_name);
            if (it != currentMembers.end()) {
                currentMembers.erase(it);
                msg = LangTty.getLocal("已从领地中移除成员: ") + player_name + " <- " + ttyname;
                needUpdate = true;
            } else {
                msg = LangTty.getLocal("无需变更成员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
                sqlite3_close(db);
                return {true, msg};  // 状态未改变，视为成功
            }
        }

        // 2. 更新成员列表：将 currentMembers 拼接成逗号分隔的字符串
        std::string updatedMembersStr = currentMembers.empty() ? "" : join(currentMembers, ',');

        std::string updateQuery = "UPDATE territories SET member=? WHERE name=?";
        sqlite3_stmt *stmtUpdate = nullptr;
        rc = sqlite3_prepare_v2(db, updateQuery.c_str(), -1, &stmtUpdate, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备更新语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备更新语句失败: " + errmsg};
        }

        rc = sqlite3_bind_text(stmtUpdate, 1, updatedMembersStr.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定更新参数失败:", errmsg);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);
            return {false, "绑定更新参数失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtUpdate, 2, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败:", errmsg);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }

        rc = sqlite3_step(stmtUpdate);
        if (rc != SQLITE_DONE) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("更新领地成员失败:", errmsg);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);
            return {false, "更新领地成员失败: " + errmsg};
        }

        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(stmtUpdate);
        sqlite3_close(db);

        if (affectedRows > 0) {
            // 更新全局领地信息
            readAllTerritories();
            return {true, msg};
        } else {
            return {false, LangTty.getLocal("更新领地成员时发生未知错误: ") + ttyname};
        }
    }

    // 领地转让函数
// 参数：
//   ttyname:        领地名称
//   old_owner_name: 原主人玩家名
//   new_owner_name: 新主人玩家名
// 返回：
//   std::pair<bool, std::string>，first 为是否成功，second 为提示信息
    std::pair<bool, std::string> change_tty_owner(const std::string &ttyname,
                                                  const std::string &old_owner_name,
                                                  const std::string &new_owner_name) {
        // 1. 检查新领地主人的领地数量是否达到上限
        if (check_tty_num(new_owner_name) >= max_tty_num) {
            std::string msg = LangTty.getLocal("玩家 ") + new_owner_name + LangTty.getLocal(" 的领地数量已达到上限, 无法增加新的领地, 转让领地失败");
            return {false, msg};
        }

        // 2. 打开数据库连接
        sqlite3* db = nullptr;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("无法打开数据库:", errmsg);
            sqlite3_close(db);
            return {false, "无法打开数据库: " + errmsg};
        }

        // 3. 查询指定领地的当前领地主人
        std::string selectQuery = "SELECT owner FROM territories WHERE name=?";
        sqlite3_stmt* stmtSelect = nullptr;
        rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmtSelect, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备查询语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备查询语句失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtSelect, 1, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败:", errmsg);
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }
        rc = sqlite3_step(stmtSelect);
        if (rc != SQLITE_ROW) {
            std::string msg = LangTty.getLocal("尝试更改领地主人但未找到领地: ") + ttyname;
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, msg};
        }
        // 获取当前 owner
        const unsigned char* text = sqlite3_column_text(stmtSelect, 0);
        std::string current_owner = text ? reinterpret_cast<const char*>(text) : "";
        sqlite3_finalize(stmtSelect);

        // 4. 判断当前领地主人是否匹配，并且新旧领主不能相同
        if (current_owner == old_owner_name && current_owner != new_owner_name) {
            // 准备更新为新主人
            current_owner = new_owner_name;
            std::string msg = LangTty.getLocal("领地转让成功, 领地主人已由 ") + old_owner_name + LangTty.getLocal(" 变更为 ") + new_owner_name;

            // 5. 执行更新操作
            std::string updateQuery = "UPDATE territories SET owner=? WHERE name=?";
            sqlite3_stmt* stmtUpdate = nullptr;
            rc = sqlite3_prepare_v2(db, updateQuery.c_str(), -1, &stmtUpdate, nullptr);
            if (rc != SQLITE_OK) {
                std::string errmsg = sqlite3_errmsg(db);
                getLogger().error("准备更新语句失败:", errmsg);
                sqlite3_close(db);
                return {false, "准备更新语句失败: " + errmsg};
            }
            rc = sqlite3_bind_text(stmtUpdate, 1, current_owner.c_str(), -1, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                std::string errmsg = sqlite3_errmsg(db);
                getLogger().error("绑定新领地主人失败:", errmsg);
                sqlite3_finalize(stmtUpdate);
                sqlite3_close(db);
                return {false, "绑定新领地主人失败: " + errmsg};
            }
            rc = sqlite3_bind_text(stmtUpdate, 2, ttyname.c_str(), -1, SQLITE_TRANSIENT);
            if (rc != SQLITE_OK) {
                std::string errmsg = sqlite3_errmsg(db);
                getLogger().error("绑定领地名失败:", errmsg);
                sqlite3_finalize(stmtUpdate);
                sqlite3_close(db);
                return {false, "绑定领地名失败: " + errmsg};
            }
            rc = sqlite3_step(stmtUpdate);
            if (rc != SQLITE_DONE) {
                std::string errmsg = sqlite3_errmsg(db);
                getLogger().error("更新领地主人失败:", errmsg);
                sqlite3_finalize(stmtUpdate);
                sqlite3_close(db);
                return {false, "更新领地主人失败: " + errmsg};
            }
            int affectedRows = sqlite3_changes(db);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);

            if (affectedRows > 0) {
                // 更新全局领地信息
                readAllTerritories();
                return {true, msg};
            } else {
                return {false, "更新领地主人时发生未知错误: " + ttyname};
            }
        } else {
            sqlite3_close(db);
            std::string msg = LangTty.getLocal("领地转让失败, 请检查是否为领地主人以及转让对象是否合规");
            return {false, msg};
        }
    }

    // 添加或删除领地管理员的函数
// 参数：
//   ttyname: 领地名称
//   action: 操作 "add" 或 "remove"
//   player_name: 目标玩家名称
// 返回：
//   std::pair<bool, std::string>，其中 first 表示操作是否成功，
//   second 为提示信息
    std::pair<bool, std::string> change_tty_manager(const std::string &ttyname,
                                                    const std::string &action,
                                                    const std::string &player_name) {
        // 检查操作类型是否合法
        if (action != "add" && action != "remove") {
            return {false, "无效的操作类型: " + action};
        }

        sqlite3* db = nullptr;
        int rc = sqlite3_open(db_file.c_str(), &db);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("无法打开数据库:", errmsg);
            sqlite3_close(db);
            return {false, "无法打开数据库: " + errmsg};
        }

        // 1. 获取当前领地管理员列表
        std::string selectQuery = "SELECT manager FROM territories WHERE name=?";
        sqlite3_stmt* stmtSelect = nullptr;
        rc = sqlite3_prepare_v2(db, selectQuery.c_str(), -1, &stmtSelect, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备查询语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备查询语句失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtSelect, 1, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败:", errmsg);
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }

        rc = sqlite3_step(stmtSelect);
        if (rc != SQLITE_ROW) {
            std::string msg = LangTty.getLocal("尝试更改领地管理员但未找到领地: ") + ttyname;
            sqlite3_finalize(stmtSelect);
            sqlite3_close(db);
            return {false, msg};
        }

        // 取得当前管理员字符串（可能为空）
        const unsigned char* text = sqlite3_column_text(stmtSelect, 0);
        std::string currentManagersStr = text ? reinterpret_cast<const char*>(text) : "";
        sqlite3_finalize(stmtSelect);

        // 拆分为管理员列表（以逗号分割）
        std::vector<std::string> currentManagers;
        if (!currentManagersStr.empty()) {
            currentManagers = split(currentManagersStr, ',');
        }

        std::string msg;
        bool needUpdate = false;

        if (action == "add" && std::find(currentManagers.begin(), currentManagers.end(), player_name) == currentManagers.end()) {
            currentManagers.push_back(player_name);
            msg = LangTty.getLocal("已添加领地管理员到领地: ") + player_name + " -> " + ttyname;
            needUpdate = true;
        } else if (action == "remove" && std::find(currentManagers.begin(), currentManagers.end(), player_name) != currentManagers.end()) {
            currentManagers.erase(std::remove(currentManagers.begin(), currentManagers.end(), player_name), currentManagers.end());
            msg = LangTty.getLocal("已从领地中移除领地管理员: ") + player_name + " <- " + ttyname;
            needUpdate = true;
        } else {
            msg = LangTty.getLocal("无需变更领地管理员: ") + player_name + LangTty.getLocal(" 在领地 ") + ttyname + LangTty.getLocal(" 中的状态未改变");
            sqlite3_close(db);
            return {true, msg};  // 状态未变，但视为成功
        }

        // 2. 将更新后的管理员列表拼接成字符串
        std::string updatedManagersStr = currentManagers.empty() ? "" : join(currentManagers, ',');

        // 3. 执行更新操作
        std::string updateQuery = "UPDATE territories SET manager=? WHERE name=?";
        sqlite3_stmt* stmtUpdate = nullptr;
        rc = sqlite3_prepare_v2(db, updateQuery.c_str(), -1, &stmtUpdate, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备更新语句失败:", errmsg);
            sqlite3_close(db);
            return {false, "准备更新语句失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtUpdate, 1, updatedManagersStr.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定更新参数失败:", errmsg);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);
            return {false, "绑定更新参数失败: " + errmsg};
        }
        rc = sqlite3_bind_text(stmtUpdate, 2, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败:", errmsg);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }

        rc = sqlite3_step(stmtUpdate);
        if (rc != SQLITE_DONE) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("更新领地管理员失败:", errmsg);
            sqlite3_finalize(stmtUpdate);
            sqlite3_close(db);
            return {false, "更新领地管理员失败: " + errmsg};
        }

        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(stmtUpdate);
        sqlite3_close(db);

        if (affectedRows > 0) {
            // 更新全局领地信息缓存
            readAllTerritories();
            return {true, msg};
        } else {
            return {false, LangTty.getLocal("更新领地管理员时发生未知错误: ") + ttyname};
        }
    }

    // 帮助将Point转换为字符串（用于提示信息）
    std::string pointToString(const Point3D &p) {
        return "(" + std::to_string(std::get<0>(p)) + ","
               + std::to_string(std::get<1>(p)) + ","
               + std::to_string(std::get<2>(p)) + ")";
    }

    // 领地传送点变更函数
// 参数：
//   ttyname: 领地名
//   tppos:   新的传送点坐标（Point类型）
//   dim:     当前所在维度
// 返回：
//   std::pair<bool, std::string>，first为操作是否成功，second为提示信息
    std::pair<bool, std::string> change_tty_tppos(const std::string &ttyname,
                                                  const Point3D &tppos,
                                                  const std::string &dim) {
        // 在更新数据库前，先检查传送点是否合法
        // 遍历全局缓存中的所有领地记录
        for (const auto &pair : all_tty) {
            const TerritoryData &territory = pair.second;
            if (territory.name == ttyname) {
                // 维度检查
                if (territory.dim != dim) {
                    std::string msg = LangTty.getLocal("你当前所在的维度(") + dim + LangTty.getLocal(")与领地维度(") + territory.dim + LangTty.getLocal(")不匹配, 无法设置领地传送点");
                    return {false, msg};
                }
                // 坐标检查
                if (!isPointInCube(tppos, territory.pos1, territory.pos2)) {
                    std::string pos_str = pointToString(tppos);
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
            getLogger().error("无法打开数据库:", errmsg);
            sqlite3_close(db);
            return {false, "无法打开数据库: " + errmsg};
        }

        // 构造SQL更新语句
        std::string update_query = "UPDATE territories SET tppos_x=?, tppos_y=?, tppos_z=? WHERE name=?";
        sqlite3_stmt* stmt = nullptr;
        rc = sqlite3_prepare_v2(db, update_query.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("准备更新语句失败:", errmsg);
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
            getLogger().error("绑定 tppos_x 参数失败:", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "绑定 tppos_x 参数失败: " + errmsg};
        }
        rc = sqlite3_bind_double(stmt, 2, y);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定 tppos_y 参数失败:", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "绑定 tppos_y 参数失败: " + errmsg};
        }
        rc = sqlite3_bind_double(stmt, 3, z);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定 tppos_z 参数失败:", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "绑定 tppos_z 参数失败: " + errmsg};
        }
        // 绑定领地名称
        rc = sqlite3_bind_text(stmt, 4, ttyname.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("绑定领地名失败:", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "绑定领地名失败: " + errmsg};
        }

        // 执行更新语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::string errmsg = sqlite3_errmsg(db);
            getLogger().error("更新领地传送点失败:", errmsg);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {false, "更新领地传送点失败: " + errmsg};
        }
        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        if (affectedRows > 0) {
            std::string pos_str = pointToString(tppos);
            std::string msg = LangTty.getLocal("已更新领地 ") + ttyname + LangTty.getLocal(" 的传送点为 ") + pos_str;
            // 更新全局领地信息缓存
            readAllTerritories();
            return {true, msg};
        } else {
            std::string msg = LangTty.getLocal("尝试更新领地传送点但未找到领地: ") + ttyname;
            return {false, msg};
        }
    }

    //插件初始化
    void onLoad() override
    {
        getLogger().info("onLoad is called");
        //执行数据目录检查
        datafile_check();
        initDatabase();
    }

    void onEnable() override
    {
        getLogger().info("onEnable is called");

        string boot_logo_msg = R"(
___________                 .__  __
\__    ___/_________________|__|/  |_  ___________ ___.__.
  |    |_/ __ \_  __ \_  __ \  \   __\/  _ \_  __ <   |  |
  |    |\  ___/|  | \/|  | \/  ||  | (  <_> )  | \/\___  |
  |____| \___  >__|   |__|  |__||__|  \____/|__|   / ____|
             \/                                    \/
)";

        getLogger().info(boot_logo_msg);
        auto enable_msg = endstone::ColorFormat::Yellow + LangTty.getLocal("Territory插件已启用, Version: ") + getServer().getPluginManager().getPlugin("territory")->getDescription().getVersion();
        json json_msg = read_config();
        try {
            if (!json_msg.contains("error")) {
                max_tty_num = json_msg["player_max_tty_num"];
                actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
            } else {
                getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
                max_tty_num = 20;
                actor_fire_attack_protect = true;
            }
        } catch (const std::exception& e) {
            max_tty_num = 20;
            actor_fire_attack_protect = true;
            getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
        }
        //注册事件监听
        registerEvent(&Territory::onBlockBreak, *this);
        registerEvent(&Territory::onBlockPlace, *this);
        registerEvent(&Territory::onActorBomb, *this);
        registerEvent(&Territory::onPlayerjiaohu, *this);
        registerEvent(&Territory::onPlayerjiaohust, *this);
        registerEvent(&Territory::onActorhit, *this);

        getLogger().info(enable_msg);
        getLogger().info(endstone::ColorFormat::Yellow + "Project address: https://github.com/yuhangle/endstone-territory");
        getLogger().info(endstone::ColorFormat::Yellow + LangTty.getLocal("请确保您的endstone版本为0.6.2及以上,否则将发生崩溃"));
        //数据库读取
        readAllTerritories();
        //周期执行
        getServer().getScheduler().runTaskTimer(*this,[&]() { tips_online_players(); }, 0, 40);

    }

    void onDisable() override
    {
        getLogger().info("onDisable is called");
    }

    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) override {
        if (command.getName() == "tty")
            if (sender.asPlayer() == nullptr) {
                getLogger().error(LangTty.getLocal("无法在服务端使用tty命令!"));
            } else {
                if (args.empty()) {
                    if (getServer().getPluginManager().getPlugin("territory_gui")) {
                        sender.asPlayer()->performCommand("ttygui");
                    } else {
                        sender.sendMessage(
                                LangTty.getLocal("安装Territory_gui插件后即可使用/ttygui命令打开图形界面菜单"));
                    }
                } else {
                    if (args[0] == "add") {
                        try {
                            string player_name = sender.getName();
                            //玩家位置
                            int player_x = getServer().getPlayer(player_name)->getLocation().getBlockX();
                            int player_y = getServer().getPlayer(player_name)->getLocation().getBlockY();
                            int player_z = getServer().getPlayer(player_name)->getLocation().getBlockZ();
                            const std::tuple<double, double, double> tppos = {player_x, player_y, player_z};
                            //领地位置1
                            Point3D pos1 = pos_to_tuple(args[1]);
                            //领地位置2
                            Point3D pos2 = pos_to_tuple(args[2]);
                            //维度
                            string dim = getServer().getPlayer(player_name)->getLocation().getDimension()->getName();
                            player_add_tty(player_name, pos1, pos2, tppos, dim);
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "add_sub") {
                        try {
                            string player_name = sender.getName();
                            //玩家位置
                            int player_x = getServer().getPlayer(player_name)->getLocation().getBlockX();
                            int player_y = getServer().getPlayer(player_name)->getLocation().getBlockY();
                            int player_z = getServer().getPlayer(player_name)->getLocation().getBlockZ();
                            const std::tuple<double, double, double> tppos = {player_x, player_y, player_z};
                            //领地位置1
                            Point3D pos1 = pos_to_tuple(args[1]);
                            //领地位置2
                            Point3D pos2 = pos_to_tuple(args[2]);
                            //维度
                            string dim = getServer().getPlayer(player_name)->getLocation().getDimension()->getName();
                            player_add_sub_tty(player_name, pos1, pos2, tppos, dim);
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "list") {
                        try {
                            string player_name = sender.getName();
                            auto territories = list_player_tty(player_name);
                            if (territories.empty()) {
                                sender.sendErrorMessage(LangTty.getLocal("你没有领地"));
                            } else {
                                std::string output_item = "";
                                for (size_t idx = 0; idx < territories.size(); ++idx) {
                                    const auto &tty = territories[idx];
                                    output_item +=
                                            std::to_string(idx + 1) + LangTty.getLocal(". 领地名称: ") + tty.name + LangTty.getLocal("\n位置范围: ") +
                                            to_string(get<0>(tty.pos1)) + " " + to_string(get<1>(tty.pos1)) + " " +
                                            to_string(get<2>(tty.pos1))
                                            + LangTty.getLocal(" 到 ") + to_string(get<0>(tty.pos2)) + " " + to_string(get<1>(tty.pos2)) +
                                            " " + to_string(get<2>(tty.pos2))
                                            + LangTty.getLocal("\n传送点位置: ") + to_string(get<0>(tty.tppos)) + " " +
                                            to_string(get<1>(tty.tppos)) + " " + to_string(get<2>(tty.tppos))
                                            + LangTty.getLocal("\n维度: ") + tty.dim + LangTty.getLocal("\n是否允许玩家交互: ") +
                                            (tty.if_jiaohu ? LangTty.getLocal("是") : LangTty.getLocal("否")) + LangTty.getLocal("\n是否允许玩家破坏: ") +
                                            (tty.if_break ? LangTty.getLocal("是") : LangTty.getLocal("否"))
                                            + LangTty.getLocal("\n是否允许外人传送: ") + (tty.if_tp ? LangTty.getLocal("是") : LangTty.getLocal("否")) +
                                            LangTty.getLocal("\n是否允许放置方块: ") + (tty.if_build ? LangTty.getLocal("是") : LangTty.getLocal("否")) + LangTty.getLocal("\n是否允许实体爆炸: ")
                                            + (tty.if_bomb ? "是" : "否") + LangTty.getLocal("\n是否允许实体伤害: ") +
                                            (tty.if_damage ? "是" : "否") + LangTty.getLocal("\n领地管理员: ") + tty.manager +
                                            LangTty.getLocal("\n领地成员: ");
                                    output_item += "\n----------------\n";
                                }
                                getServer().getPlayer(sender.getName())->sendMessage(output_item);
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "del") {
                        try {
                            string player_name = sender.getName();
                            if (args[1] != "") {
                                if (read_territory_by_name(args[1]) != nullptr) {
                                    if (check_tty_owner(args[1], player_name) == true) {
                                        if (del_player_tty(args[1])) {
                                            sender.sendMessage(LangTty.getLocal("已成功删除领地"));
                                        } else {
                                            sender.sendErrorMessage(LangTty.getLocal("删除领地失败"));
                                        }
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有权限删除此领地"));
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                }


                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "rename") {
                        try {
                            string player_name = sender.getName();
                            string tty_name = args[1];
                            TerritoryData *tty_data = read_territory_by_name(tty_name);
                            if (tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                if (args[1] != "" && args[2] != "") {
                                    if (check_tty_owner(args[1], player_name) == true) {
                                        pair status_msg = rename_player_tty(args[1], args[2]);
                                        if (status_msg.first) {
                                            sender.sendMessage(status_msg.second);
                                        } else {
                                            sender.sendErrorMessage(status_msg.second);
                                        }
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有权限重命名此领地"));
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                                }
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "set") {
                        try {
                            string player_name = sender.getName();
                            if (args[1] != "" && args[2] != "" && args[3] != "") {
                                string tty_name = args[3];
                                TerritoryData *tty_data = read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (check_tty_op(tty_name, player_name) == true) {
                                        int per_val;
                                        if (args[2] == "true") {
                                            per_val = 1;
                                        } else {
                                            per_val = 0;
                                        }
                                        pair status_msg = change_tty_permissions(tty_name, args[1], per_val);
                                        if (status_msg.first) {
                                            sender.sendMessage(status_msg.second);
                                        } else {
                                            sender.sendErrorMessage(status_msg.second);
                                        }
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地"));
                                    }
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "member") {
                        try {
                            string player_name = sender.getName();
                            if (args[1] != "" && args[2] != "" && args[3] != "") {
                                string tty_name = args[3];
                                TerritoryData *tty_data = read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (check_tty_op(tty_name, player_name) == true) {
                                        if (args[1] == "add") {
                                            pair status_msg = change_tty_member(tty_name, "add", args[2]);
                                            if (status_msg.first) {
                                                sender.sendMessage(status_msg.second);
                                            } else {
                                                sender.sendErrorMessage(status_msg.second);
                                            }
                                        } else if (args[1] == "remove") {
                                            pair status_msg = change_tty_member(tty_name, "remove", args[2]);
                                            if (status_msg.first) {
                                                sender.sendMessage(status_msg.second);
                                            } else {
                                                sender.sendErrorMessage(status_msg.second);
                                            }
                                        }
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地"));
                                    }
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "manager") {
                        try {
                            string player_name = sender.getName();
                            if (args[1] != "" && args[2] != "" && args[3] != "") {
                                string tty_name = args[3];
                                TerritoryData *tty_data = read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (check_tty_owner(tty_name, player_name) == true) {
                                        if (args[1] == "add") {
                                            pair status_msg = change_tty_manager(tty_name, "add", args[2]);
                                            if (status_msg.first) {
                                                sender.sendMessage(status_msg.second);
                                            } else {
                                                sender.sendErrorMessage(status_msg.second);
                                            }
                                        } else if (args[1] == "remove") {
                                            pair status_msg = change_tty_manager(tty_name, "remove", args[2]);
                                            if (status_msg.first) {
                                                sender.sendMessage(status_msg.second);
                                            } else {
                                                sender.sendErrorMessage(status_msg.second);
                                            }
                                        }
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地"));
                                    }
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "settp") {
                        try {
                            if (args[1] != "" && args[2] != "") {
                                string player_name = sender.getName();
                                string tty_name = args[2];
                                string dim = getServer().getPlayer(
                                        player_name)->getLocation().getDimension()->getName();
                                if (check_tty_op(tty_name, player_name) == true) {
                                    Point3D tp_pos = pos_to_tuple(args[1]);
                                    pair status_msg = change_tty_tppos(tty_name, tp_pos, dim);
                                    if (status_msg.first) {
                                        sender.sendMessage(status_msg.second);
                                    } else {
                                        sender.sendErrorMessage(status_msg.second);
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地"));
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "transfer") {
                        try {
                            string player_name = sender.getName();
                            if (args[1] != "" && args[2] != "") {
                                string tty_name = args[1];
                                TerritoryData *tty_data = read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (check_tty_owner(tty_name, player_name) == true) {
                                        pair status_msg = change_tty_owner(tty_name, player_name, args[2]);
                                        if (status_msg.first) {
                                            sender.sendMessage(status_msg.second);
                                        } else {
                                            sender.sendErrorMessage(status_msg.second);
                                        }
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地"));
                                    }
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "tp") {
                        try {
                            if (args[1] != "") {
                                string player_name = sender.getName();
                                string tty_name = args[1];
                                TerritoryData *tty_info = read_territory_by_name(tty_name);
                                if (tty_info != nullptr) {
                                    if (tty_info->if_tp) {
                                        auto player = getServer().getPlayer(player_name);
                                        auto tty_Dim = getServer().getLevel()->getDimension(tty_info->dim);
                                        endstone::Location loc(tty_Dim, static_cast<float>(get<0>(tty_info->tppos)),
                                                               static_cast<float>(get<1>(tty_info->tppos)),
                                                               static_cast<float>(get<2>(tty_info->tppos)),
                                                               0.0, 0.0);

                                        player->teleport(loc);
                                    } else if (tty_info->member.find(player_name) != std::string::npos ||
                                               tty_info->manager.find(player_name) != std::string::npos ||
                                               player_name == tty_info->owner) {
                                        auto player = getServer().getPlayer(player_name);
                                        auto tty_Dim = getServer().getLevel()->getDimension(tty_info->dim);
                                        endstone::Location loc(tty_Dim, static_cast<float>(get<0>(tty_info->tppos)),
                                                               static_cast<float>(get<1>(tty_info->tppos)),
                                                               static_cast<float>(get<2>(tty_info->tppos)),
                                                               0.0, 0.0);
                                        player->teleport(loc);
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("你没有传送到此领地的权限"));
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                            }
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "help") {
                        try {
                            string player_name = sender.getName();
                            string help_info = LangTty.getLocal("新建领地--/tty add 领地边角坐标1 领地边角坐标2\n新建子领地--/tty add_sub 子领地边角坐标1 子领地边角坐标2\n列出领地--/tty list\n删除领地--/tty del 领地名\n重命名领地--/tty rename 旧领地名 新领地名\n设置领地权限--/tty set 权限名(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage) 权限值 领地名\n设置领地管理员--/tty manager add|remove(添加|删除) 玩家名 领地名\n设置领地成员--/tty member add|remove(添加|删除) 玩家名 领地名\n设置领地传送点--/tty settp 领地传送坐标 领地名\n传送领地--/tty tp 领地名\n");
                            sender.sendMessage(help_info);
                        } catch (const std::runtime_error &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    }
                }
            }
        else if (command.getName() == "optty") {
            if (sender.asPlayer() == nullptr) {
                if (args.empty()) {
                    getLogger().error(LangTty.getLocal("缺少参数"));
                    return false;
                }
                if (args[0] == "del") {
                    if (args[1] != "") {
                        if (read_territory_by_name(args[1]) != nullptr) {
                            if (del_player_tty(args[1])) {
                                getLogger().info(LangTty.getLocal("已删除领地"));
                            } else {
                                getLogger().error(LangTty.getLocal("领地删除失败"));
                            }
                        } else {
                            getLogger().error(LangTty.getLocal("未知的领地"));
                        }
                    } else {
                        getLogger().error(LangTty.getLocal("缺少参数"));
                    }
                } else if (args[0] == "del_all") {
                    if (args[1] != "") {
                        auto tty_info = list_player_tty(args[1]);
                        if (tty_info.empty()) {
                            getLogger().error(LangTty.getLocal("该玩家没有领地"));
                        } else {
                            getLogger().info(
                                    LangTty.getLocal("正在删除玩家 ") + args[1] + LangTty.getLocal(" 的全部领地"));
                            for (const auto &the_tty: tty_info) {
                                if (del_player_tty(the_tty.name)) {
                                    getLogger().info(LangTty.getLocal("成功删除领地: ") + the_tty.name);
                                } else {
                                    getLogger().error(LangTty.getLocal("删除领地失败: ") + the_tty.name);
                                }
                            }
                        }
                    } else {
                        getLogger().error(LangTty.getLocal("缺少参数"));
                    }
                } else if (args[0] == "reload") {
                    readAllTerritories();
                    json json_msg = read_config();
                    try {
                        if (!json_msg.contains("error")) {
                            max_tty_num = json_msg["player_max_tty_num"];
                            actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
                        } else {
                            getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
                            max_tty_num = 20;
                            actor_fire_attack_protect = true;
                        }
                    } catch (const std::exception &e) {
                        max_tty_num = 20;
                        actor_fire_attack_protect = true;
                        getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
                    }
                    getLogger().info(LangTty.getLocal("重载领地配置和数据完成"));
                } else if (args[0] == "set") {
                    try {
                        string player_name = sender.getName();
                        if (args[1] != "" && args[2] != "" && args[3] != "") {
                            string tty_name = args[3];
                            TerritoryData *tty_data = read_territory_by_name(tty_name);
                            if (tty_data == nullptr) {
                                getLogger().error(LangTty.getLocal("未知的领地"));
                            } else {
                                int per_val;
                                if (args[2] == "true") {
                                    per_val = 1;
                                } else {
                                    per_val = 0;
                                }
                                pair status_msg = change_tty_permissions(tty_name, args[1], per_val);
                                if (status_msg.first) {
                                    getLogger().info(status_msg.second);
                                } else {
                                    getLogger().error(status_msg.second);
                                }
                            }
                        } else {
                            getLogger().error(LangTty.getLocal("缺少参数"));
                        }
                    } catch (const std::runtime_error &e) {
                        getLogger().error(e.what());
                    }
                }
                //玩家部分
            } else {
                if (args.empty()) {
                    sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                    return false;
                }
                if (args[0] == "del") {
                    if (args[1] != "") {
                        if (read_territory_by_name(args[1]) != nullptr) {
                            if (del_player_tty(args[1])) {
                                sender.sendMessage(LangTty.getLocal("已删除领地"));
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("领地删除失败"));
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                        }
                    } else {
                        sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                    }
                } else if (args[0] == "del_all") {
                    if (args[1] != "") {
                        auto tty_info = list_player_tty(args[1]);
                        if (tty_info.empty()) {
                            sender.sendErrorMessage(LangTty.getLocal("该玩家没有领地"));
                        } else {
                            sender.sendMessage(
                                    LangTty.getLocal("正在删除玩家 ") + args[1] + LangTty.getLocal(" 的全部领地"));
                            for (const auto &the_tty: tty_info) {
                                if (del_player_tty(the_tty.name)) {
                                    sender.sendMessage(LangTty.getLocal("成功删除领地: ") + the_tty.name);
                                } else {
                                    sender.sendErrorMessage("删除领地失败: " + the_tty.name);
                                }
                            }
                        }
                    } else {
                        sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                    }
                } else if (args[0] == "reload") {
                    readAllTerritories();
                    json json_msg = read_config();
                    try {
                        if (!json_msg.contains("error")) {
                            max_tty_num = json_msg["player_max_tty_num"];
                            actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("配置文件错误,使用默认配置"));
                            max_tty_num = 20;
                            actor_fire_attack_protect = true;
                        }
                    } catch (const std::exception &e) {
                        max_tty_num = 20;
                        actor_fire_attack_protect = true;
                        sender.sendErrorMessage(LangTty.getLocal("配置文件错误,使用默认配置"));
                    }
                    sender.sendMessage(LangTty.getLocal("重载领地配置和数据完成"));
                } else if (args[0] == "set") {
                    try {
                        string player_name = sender.getName();
                        if (args[1] != "" && args[2] != "" && args[3] != "") {
                            string tty_name = args[3];
                            TerritoryData *tty_data = read_territory_by_name(tty_name);
                            if (tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                int per_val;
                                if (args[2] == "true") {
                                    per_val = 1;
                                } else {
                                    per_val = 0;
                                }
                                pair status_msg = change_tty_permissions(tty_name, args[1], per_val);
                                if (status_msg.first) {
                                    sender.sendMessage(status_msg.second);
                                } else {
                                    sender.sendErrorMessage(status_msg.second);
                                }
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                        }
                    } catch (const std::runtime_error &e) {
                        sender.sendErrorMessage(e.what());
                    }
                }
            }
        }
        return true;
    }

    //方块破坏监听
    void onBlockBreak(endstone::BlockBreakEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Point3D block_pos = {event.getBlock().getLocation().getBlockX(),event.getBlock().getLocation().getBlockY(),event.getBlock().getLocation().getBlockZ()};
        auto player_in_tty = list_in_tty(block_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_break)) {
                    if (find(info.members.begin(),info.members.end(),player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上破坏的权限"));
                        return;
                    }
                }
            }
        }
    }

    //方块放置监听
    void onBlockPlace(endstone::BlockPlaceEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Point3D block_pos = {event.getBlock().getLocation().getBlockX(),event.getBlock().getLocation().getBlockY(),event.getBlock().getLocation().getBlockZ()};
        auto player_in_tty = list_in_tty(block_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_build)) {
                    if (find(info.members.begin(),info.members.end(),player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上放置的权限"));
                        return;
                    }
                }
            }
        }
    }

    //玩家交互监听
    void onPlayerjiaohu(endstone::PlayerInteractEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Point3D block_pos = {event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
        auto player_in_tty = list_in_tty(block_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_jiaohu)) {
                    if (find(info.members.begin(),info.members.end(),player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上交互的权限"));
                        return;
                    }
                }
            }
        }
    }

    //玩家实体交互监听
    void onPlayerjiaohust(endstone::PlayerInteractActorEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
        auto player_in_tty = list_in_tty(actor_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_jiaohu)) {
                    if (find(info.members.begin(),info.members.end(),player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上交互的权限"));
                        return;
                    }
                }
            }
        }
    }

    //实体爆炸监听
    void onActorBomb(endstone::ActorExplodeEvent& event)
    {
        string actor_dim = event.getActor().getLocation().getDimension()->getName();
        Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
        auto actor_in_tty = list_in_tty(actor_pos,actor_dim);
        //检查实体是否在领地上
        if (actor_in_tty != std::nullopt) {
            for (const auto&info : actor_in_tty.value()) {
                if (!(info.if_bomb)) {
                    event.setCancelled(true);
                    return;
                }
            }
        }
    }

    //实体受击
    void onActorhit(endstone::ActorDamageEvent& event)
    {
        string actor_dim = event.getActor().getLocation().getDimension()->getName();
        Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
        auto actor_in_tty = list_in_tty(actor_pos,actor_dim);
        //检查实体是否在领地上
        if (actor_in_tty != std::nullopt) {
            for (const auto&info : actor_in_tty.value()) {
                if (!(info.if_damage)) {
                    auto actor_or_player = event.getDamageSource().getActor();
                    // 玩家导致伤害
                    if (actor_or_player && actor_or_player->getType() == "minecraft:player") {
                        if (find(info.members.begin(),info.members.end(),actor_or_player->getName()) == info.members.end()) {
                            actor_or_player->sendErrorMessage(LangTty.getLocal("你没有在此领地上伤害的权限"));
                            event.setCancelled(true);
                        }
                        //火焰攻击类伤害
                    } else if (event.getDamageSource().getType() == "fire_tick") {
                        //非玩家实体才免疫
                        if (!(event.getActor().getType() == "minecraft:player")) {
                            if (actor_fire_attack_protect == true) {
                                event.setCancelled(true);
                            }
                        }
                    }
                    return;
                }
            }
        }
    }

};
