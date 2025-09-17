//
// Created by yuhang on 2025/9/3.
//

#ifndef TERRITORY_TERRITORYCORE_H
#define TERRITORY_TERRITORYCORE_H

#include "DataBase.h"
#include <endstone/endstone.hpp>
#include <endstone/plugin/plugin.h>
#include <endstone/color_format.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <tuple>
#include <utility>
#include <algorithm>
#include <endstone/player.h>
#include <endstone/server.h>
#include <sstream>
#include <vector>
#include <optional>
#include <endstone/level/position.h>
#include <endstone/actor/actor.h>
#include <endstone/event/block/block_break_event.h>
#include <endstone/event/block/block_place_event.h>
#include <endstone/event/actor/actor_explode_event.h>
#include <endstone/event/player/player_interact_event.h>
#include <endstone/event/player/player_interact_actor_event.h>
#include <endstone/event/actor/actor_damage_event.h>
#include "translate.h"
#include <random>
#include <iomanip>
#include "Territory_Action.h"
#include "menu.h"

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

//数据文件路径
inline std::string data_path = "plugins/territory";
inline std::string config_path = "plugins/territory/config.json";
inline const std::string db_file = "plugins/territory/territory_data.db";
const std::string umoney_file = "plugins/umoney/money.json";

//一些全局变量
inline int max_tty_num;
inline bool actor_fire_attack_protect;
inline bool money_with_umoney;
inline int price;
inline int max_tty_area;
inline bool welcome_all;

//初始化其它实例
inline DataBase Database(db_file);
inline Territory_Action TA(Database);

//全局玩家位置信息
inline std::unordered_map<std::string, std::tuple<Territory_Action::Point3D, string, string>> lastPlayerPositions;

//快速创建领地玩家数据缓存
inline std::unordered_map<std::string,Territory_Action::QuickTtyData> quick_create_player_data;

class Territory : public endstone::Plugin {


public:

    // 读取配置文件
    [[nodiscard]] json read_config() const {
        std::ifstream i(config_path);
        try {
            json j;
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
    void datafile_check() const {
        json df_config = {
                {"player_max_tty_num", 20},
                {"actor_fire_attack_protect", true},
                {"money_with_umoney", false},
                {"price", 1},
                {"max_tty_area",4000000},
                {"welcome_all",false}
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
                        getLogger().info(LangTty.getLocal("配置项 '{}' 已根据默认配置进行更新")+","+ key);
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

    // 从数据库读取所有领地数据
    static void readAllTerritories() {
        (void)TA.get_all_tty();
    }
    
    //添加领地函数
    void player_add_tty(const std::string& player_name, const Territory_Action::Point3D &pos1, const Territory_Action::Point3D &pos2, const Territory_Action::Point3D &tppos, const std::string& dim) const {
        int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
        //检查领地大小
        if (area >= max_tty_area && max_tty_area != -1) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建"));
            return;
        }
        // 检查玩家领地数量是否达到上限
        if (Territory_Action::check_tty_num(player_name) >= max_tty_num) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地"));
            return;
        }
        //检查领地是否为一个点
        if (pos1 == pos2) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("无法设置领地为一个点"));
            return;
        }
        // 检查新领地是否与其他领地重叠
        if (Territory_Action::isTerritoryOverlapping(pos1, pos2, dim)) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("此区域与其他玩家领地重叠"));
            return;
        }
        // 检查传送点是否在领地内
        if (!Territory_Action::isPointInCube(tppos, pos1, pos2)) {
            getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("你当前所在的位置不在你要添加的领地上!禁止远程施法"));
            return;
        }
        else {
            //检查是否启用经济以及资产是否足够
            if (money_with_umoney) {
                int money = umoney_get_player_money(player_name);
                int value = area * price;
                if (money >= value) {
                    (void)umoney_change_player_money(player_name,-value);
                    getServer().getPlayer(player_name)->sendMessage(LangTty.getLocal("设置领地已扣费:") + to_string(value));
                } else {
                    getServer().getPlayer(player_name)->sendErrorMessage(LangTty.getLocal("你的资产不足以设置此大小的领地,设置此领地所需要的资金为:") +
                                                                                 to_string(value));
                    return;
                }
            }
            // 使用当前时间作为领地名
            // 使用当前时间和父领地名作为新领地名
            auto now = std::chrono::system_clock::now();
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
#ifdef _WIN32
            // Windows
            struct tm timeinfo{};
            if (localtime_s(&timeinfo, &now_time_t) == 0) {
                ss << std::put_time(&timeinfo, "%Y%m%d%H%M%S");
            } else {
                ss << generateUUID();
            }
#else
            // Linux
            struct tm timeinfo{};
            if (localtime_r(&now_time_t, &timeinfo) != nullptr) {
                ss << std::put_time(&timeinfo, "%Y%m%d%H%M%S");
            } else {
                ss << generateUUID();
            }
#endif
            //std::time_t current_time = std::chrono::system_clock::to_time_t(now);
            std::string name = ss.str();

            // 插入新领地数据到数据库
            (void)Database.addTerritory(name,
                       std::get<0>(pos1), std::get<1>(pos1), std::get<2>(pos1),
                       std::get<0>(pos2), std::get<1>(pos2), std::get<2>(pos2),
                       std::get<0>(tppos), std::get<1>(tppos), std::get<2>(tppos),
                       player_name, "", "", false, false, false, false, false,false, dim, "");

            // 向玩家发送成功消息
            getServer().getPlayer(player_name)->sendMessage(LangTty.getLocal("成功添加领地"));

            // 更新全局数据
            (void)readAllTerritories();
        }
    }

    //用于添加玩家子领地的函数
    [[nodiscard]] std::pair<bool, std::string> player_add_sub_tty(const std::string& playername,
                                                   const Territory_Action::Point3D& pos1,
                                                   const Territory_Action::Point3D& pos2,
                                                   const Territory_Action::Point3D& tppos,
                                                   const std::string& dim) const {
        int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
        //检查领地大小
        if (area >= max_tty_area && max_tty_area != -1) {
            getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建"));
            return{false,""};
        }
        // 检查玩家领地数量是否已达上限
        if (Territory_Action::check_tty_num(playername) >= max_tty_num) {
            getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地"));
            return {false, ""};
        }
        //检查领地是否为一个点
        if (pos1 == pos2) {
            getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("无法设置领地为一个点"));
            return {false,""};
        }
        // 检查传送点是否在领地内
        if (!Territory_Action::isPointInCube(tppos, pos1, pos2)) {
            getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("你当前所在的位置不在你要添加的领地上!禁止远程施法"));
            return {false, ""};
        }

        // 父领地检查
        auto fatherTTYInfo = Territory_Action::listTrueFatherTTY(playername, std::make_tuple(pos1, pos2), dim);
        if (!fatherTTYInfo.first) {
            getServer().getPlayer(playername)->sendErrorMessage(fatherTTYInfo.second);
            return {false, ""};
        } else {
            //检查是否启用经济以及资产是否足够
            if (money_with_umoney) {
                int money = umoney_get_player_money(playername);
                int value = area * price;
                if (money >= value) {
                    (void)umoney_change_player_money(playername,-value);
                    getServer().getPlayer(playername)->sendMessage(LangTty.getLocal("设置领地已扣费:") + to_string(value));
                } else {
                    getServer().getPlayer(playername)->sendErrorMessage(LangTty.getLocal("你的资产不足以设置此大小的领地,设置此领地所需要的资金为:") +
                                                                         to_string(value));
                    return {false,""};
                }
            }
            // 使用当前时间和父领地名作为新领地名
            auto now = std::chrono::system_clock::now();
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
#ifdef _WIN32
            // Windows
            struct tm timeinfo{};
            if (localtime_s(&timeinfo, &now_time_t) == 0) {
                ss << std::put_time(&timeinfo, "%Y%m%d%H%M%S");
            } else {
                ss << generateUUID();
            }
#else
            // Linux
            struct tm timeinfo{};
            if (localtime_r(&now_time_t, &timeinfo) != nullptr) {
                ss << std::put_time(&timeinfo, "%Y%m%d%H%M%S");
            } else {
                ss << generateUUID();
            }
#endif

            std::string childTTYName = fatherTTYInfo.second + "_" + ss.str();

            // 创建新领地数据结构
            Territory_Action::TerritoryData newTTYData;
            newTTYData.name = childTTYName;
            newTTYData.pos1 = pos1;
            newTTYData.pos2 = pos2;
            newTTYData.tppos = tppos;
            newTTYData.owner = playername;
            newTTYData.dim = dim;
            newTTYData.father_tty = fatherTTYInfo.second;

            // 写入数据库
            (void)Database.addTerritory(childTTYName,
                           get<0>(pos1),get<1>(pos1),get<2>(pos1),
                           get<0>(pos2),get<1>(pos2),get<2>(pos2),
                           get<0>(tppos),get<1>(tppos),get<2>(tppos),playername,"","",
                           false,false,false,false,false,false,dim,fatherTTYInfo.second);
            getServer().getPlayer(playername)->sendMessage(LangTty.getLocal("成功添加子领地,归属于父领地") + fatherTTYInfo.second);
            // 更新全局数据
            (void)readAllTerritories();
            return {true, childTTYName};
        }
    }

    // 提示领地信息函数
    void tips_online_players() const {
        auto online_player_list = getServer().getOnlinePlayers();

        for (const auto &player : online_player_list) {
            string player_name = player->getName();
            string player_dim = player->getLocation().getDimension()->getName();
            Territory_Action::Point3D player_pos = {player->getLocation().getBlockX(), player->getLocation().getBlockY(),
                                  player->getLocation().getBlockZ()};

            // 检查玩家位置是否有变化
            if (!welcome_all && get<0>(lastPlayerPositions[player_name]) == player_pos) {
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
            const auto& all_tty = Territory_Action::getAllTty();
            // 遍历所有领地数据，确定玩家所在的最精细的领地（若存在子领地，则取子领地）
            const Territory_Action::TerritoryData* selectedTerritory = nullptr;
            for (const auto &val: all_tty | views::values) {
                const Territory_Action::TerritoryData &data = val;
                if (data.dim == player_dim && Territory_Action::isPointInCube(player_pos, data.pos1, data.pos2)) {
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
                if (previous_territory != current_territory || welcome_all) {
                    std::string msg;
                    if (!current_father_territory.empty()) {
                        // 进入子领地
                        if (welcome_all) {
                            msg = LangTty.getLocal("§2[领地] §r您当前位于 ") + selectedTerritory->owner + LangTty.getLocal(" 的子领地 ") + current_territory;
                        } else {
                            msg = LangTty.getLocal("§2[领地] §r欢迎来到 ") + selectedTerritory->owner + LangTty.getLocal(" 的子领地 ") + current_territory;
                        }
                    } else {
                        // 进入普通领地
                        if (welcome_all) {
                            msg = LangTty.getLocal("§2[领地] §r您当前位于 ") + selectedTerritory->owner + LangTty.getLocal(" 的领地 ") + current_territory;
                        } else {
                            msg = LangTty.getLocal("§2[领地] §r欢迎来到 ") + selectedTerritory->owner + LangTty.getLocal(" 的领地 ") + current_territory;
                        }
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

    // 删除玩家领地函数，删除名称为 tty_name 的领地，并更新相关数据
    [[nodiscard]] bool del_player_tty(const std::string &tty_name) const {
        if (TA.del_Tty_by_name(tty_name)) {
            if (money_with_umoney) {
                auto tty_data = Territory_Action::read_territory_by_name(tty_name);
                int area = Territory_Action::get_tty_area(static_cast<int>(get<0>(tty_data->pos1)),static_cast<int>(get<2>(tty_data->pos1)),static_cast<int>(get<0>(tty_data->pos2)),static_cast<int>(get<2>(tty_data->pos2)));
                (void)umoney_change_player_money(tty_data->owner,area*price);
                if (auto the_player = getServer().getPlayer(tty_data->owner)) {
                    the_player->sendMessage(LangTty.getLocal("您的领地已被删除,以当前价格返还资金:") + to_string(area*price));
                }
            }
            return true;
        } else {
            return false;
        }
    }

    //UUID生成
    static std::string generateUUID() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        constexpr char hex_chars[] = "0123456789abcdef";

        std::stringstream ss;
        for (int i = 0; i < 8; ++i) ss << hex_chars[dis(gen)];
        ss << "-";
        for (int i = 0; i < 4; ++i) ss << hex_chars[dis(gen)];
        ss << "-4"; // UUID version 4
        for (int i = 0; i < 3; ++i) ss << hex_chars[dis(gen)];
        ss << "-";
        ss << hex_chars[8 + dis(gen) % 4]; // Variant 10xx
        for (int i = 0; i < 3; ++i) ss << hex_chars[dis(gen)];
        ss << "-";
        for (int i = 0; i < 12; ++i) ss << hex_chars[dis(gen)];

        return ss.str();
    }

    //接入umoney

    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const {
        if (getServer().getPluginManager().getPlugin("umoney")) {
            return true;
        } else {
            return false;
        }
    }
    //获取玩家资金
    static int umoney_get_player_money(const std::string& player_name) {
        std::ifstream f(umoney_file);
        if (!f.is_open()) {
            std::cerr << "Error: Could not open file: " << umoney_file << std::endl;
            return -1; // 返回一个错误值表示文件无法打开
        }

        try {
            json data = json::parse(f);
            if (data.contains(player_name)) {
                return data[player_name].get<int>();
            } else {
                std::cerr << "Warning: Player '" << player_name << "' not found in the data." << std::endl;
                return 0; // 返回 0 表示玩家不存在或没有资金记录
            }
        } catch (const json::parse_error& e) {
            std::cerr << "Error: JSON parse error in file '" << umoney_file << "': " << e.what() << std::endl;
            return -1; // 返回一个错误值表示 JSON 解析失败
        }

        return -1;
    }
//更改玩家资金
    [[nodiscard]] bool umoney_change_player_money(const std::string& player_name, const int money) const {
        std::ifstream ifs(umoney_file);
        if (!ifs.is_open()) {
            std::cerr << "Error: Could not open file for reading: " << umoney_file << std::endl;
            return false;
        }
        //优先使用money_connect插件的命令操作umoney
        if (getServer().getPluginManager().getPlugin("money_connect")) {
            string command = "myct umoney change \"" + player_name + "\" " + to_string(money);
            return getServer().dispatchCommand(getServer().getCommandSender(),command);
        }
        //未能使用money_connect,直接数据操作
        try {
            json data = json::parse(ifs);
            ifs.close(); // 关闭读取流

            if (data.contains(player_name)) {
                data[player_name] = data[player_name].get<int>() + money;
            } else {
                // 如果玩家不存在，则创建一个新记录
                data[player_name] = money;
            }

            std::ofstream ofs(umoney_file);
            if (!ofs.is_open()) {
                std::cerr << "Error: Could not open file for writing: " << umoney_file << std::endl;
                return false;
            }
            ofs << data.dump(4); // 将修改后的 JSON 写回文件
            ofs.close();

            return true;

        } catch (const json::parse_error& e) {
            std::cerr << "Error: JSON parse error in file '" << umoney_file << "': " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Error: An unexpected error occurred: " << e.what() << std::endl;
            return false;
        }

        return false;
    }


    //插件初始化
    void onLoad() override
    {
        getLogger().info("onLoad is called");
        //执行数据目录检查
        datafile_check();
        (void)Database.init_database();
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
        auto lang_status = LangTty.loadLanguage();
        getLogger().info(lang_status.second);
        auto enable_msg = endstone::ColorFormat::Yellow + LangTty.getLocal("Territory插件已启用, Version: ") + getServer().getPluginManager().getPlugin("territory")->getDescription().getVersion();
        json json_msg = read_config();
        try {
            if (!json_msg.contains("error")) {
                max_tty_num = json_msg["player_max_tty_num"];
                actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
                max_tty_area = json_msg["max_tty_area"];
                welcome_all = json_msg["welcome_all"];
                if (json_msg["money_with_umoney"]) {
                    if (json_msg["money_with_umoney"] && umoney_check_exists() && json_msg["price"] >0) {
                        money_with_umoney = true;
                        price = json_msg["price"];
                    } else {
                        money_with_umoney = false;
                        price = 1;
                        getLogger().error(LangTty.getLocal("经济配置错误,检查umoney插件是否安装,或者价格是否大于0;领地价格依然保持默认"));
                    }
                } else {
                    money_with_umoney = false;
                    price = 1;
                }
            } else {
                getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
                max_tty_num = 20;
                actor_fire_attack_protect = true;
                money_with_umoney = false;
                price = 1;
                max_tty_area = 4000000;
                welcome_all = false;
            }
        } catch (const std::exception& e) {
            max_tty_num = 20;
            actor_fire_attack_protect = true;
            money_with_umoney = false;
            price = 1;
            max_tty_area = 4000000;
            welcome_all = false;
            getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置")+","+e.what());
        }
        //注册事件监听
        registerEvent<endstone::BlockBreakEvent>(Territory::onBlockBreak);
        registerEvent<endstone::BlockPlaceEvent>(Territory::onBlockPlace);
        registerEvent<endstone::ActorExplodeEvent>(Territory::onActorBomb);
        registerEvent<endstone::PlayerInteractEvent>(Territory::onPlayerjiaohu);
        registerEvent<endstone::PlayerInteractActorEvent>(Territory::onPlayerjiaohust);
        registerEvent<endstone::ActorDamageEvent>(Territory::onActorhit);
        //快速创建领地选择监听
        registerEvent<endstone::PlayerInteractEvent>(quickCreateTtyRightClick);

        getLogger().info(enable_msg);
        getLogger().info(endstone::ColorFormat::Yellow + "Project address: https://github.com/yuhangle/endstone-territory");
        //数据库读取
        (void)readAllTerritories();
        //周期执行
        getServer().getScheduler().runTaskTimer(*this,[&]() { tips_online_players(); }, 0, 25);
        menu_ = std::make_unique<Menu>(*this);

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
                    //菜单
                    if (getServer().getPluginManager().getPlugin("territory_gui")) {
                        sender.sendMessage(LangTty.getLocal("Territory_gui插件已不再需要，菜单已被集成到插件本体内"));
                        menu_->openMainMenu(sender.asPlayer());
                    } else {
                        menu_->openMainMenu(sender.asPlayer());
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
                            Territory_Action::Point3D pos1 = Territory_Action::pos_to_tuple(args[1]);
                            //领地位置2
                            Territory_Action::Point3D pos2 = Territory_Action::pos_to_tuple(args[2]);
                            if (get<1>(pos1) > 320 || get<1>(pos1) < -64 || get<1>(pos2) > 320 || get<1>(pos2) < -64) {
                                sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地"));
                                return false;
                            }
                            //维度
                            string dim = getServer().getPlayer(player_name)->getLocation().getDimension()->getName();
                            player_add_tty(player_name, pos1, pos2, tppos, dim);
                        } catch (const std::exception &e) {
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
                            Territory_Action::Point3D pos1 = Territory_Action::pos_to_tuple(args[1]);
                            //领地位置2
                            Territory_Action::Point3D pos2 = Territory_Action::pos_to_tuple(args[2]);
                            if (get<1>(pos1) > 320 || get<1>(pos1) < -64 || get<1>(pos2) > 320 || get<1>(pos2) < -64) {
                                sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地"));
                                return false;
                            }
                            //维度
                            string dim = getServer().getPlayer(player_name)->getLocation().getDimension()->getName();
                            (void)player_add_sub_tty(player_name, pos1, pos2, tppos, dim);
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "list") {
                        try {
                            string player_name = sender.getName();
                            auto territories = Territory_Action::list_player_tty(player_name);
                            if (territories.empty()) {
                                sender.sendErrorMessage(LangTty.getLocal("你没有领地"));
                            } else {
                                std::string output_item;
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
                                            + (tty.if_bomb ? LangTty.getLocal("是") : LangTty.getLocal("否")) + LangTty.getLocal("\n是否允许实体伤害: ") +
                                            (tty.if_damage ? LangTty.getLocal("是") : LangTty.getLocal("否")) + LangTty.getLocal("\n领地管理员: ") + tty.manager +
                                            LangTty.getLocal("\n领地成员: ");
                                    output_item += "\n----------------\n";
                                }
                                getServer().getPlayer(sender.getName())->sendMessage(output_item);
                            }
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "del") {
                        try {
                            string player_name = sender.getName();
                            if (!args[1].empty()) {
                                if (Territory_Action::read_territory_by_name(args[1]) != nullptr) {
                                    if (Territory_Action::check_tty_owner(args[1], player_name) == true) {
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "rename") {
                        try {
                            string player_name = sender.getName();
                            const string& tty_name = args[1];
                            Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                            if (tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                if (!args[1].empty() && !args[2].empty()) {
                                    if (Territory_Action::check_tty_owner(args[1], player_name) == true) {
                                        pair status_msg = TA.rename_player_tty(args[1], args[2]);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "set") {
                        try {
                            string player_name = sender.getName();
                            if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                                const string& tty_name = args[3];
                                Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (Territory_Action::check_tty_op(tty_name, player_name) == true) {
                                        int per_val;
                                        if (args[2] == "true") {
                                            per_val = 1;
                                        } else {
                                            per_val = 0;
                                        }
                                        pair status_msg = TA.change_territory_permissions(tty_name, args[1], per_val);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "member") {
                        try {
                            string player_name = sender.getName();
                            if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                                const string& tty_name = args[3];
                                Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (Territory_Action::check_tty_op(tty_name, player_name) == true) {
                                        if (args[1] == "add") {
                                            pair status_msg = TA.change_territory_member(tty_name, "add", args[2]);
                                            if (status_msg.first) {
                                                sender.sendMessage(status_msg.second);
                                            } else {
                                                sender.sendErrorMessage(status_msg.second);
                                            }
                                        } else if (args[1] == "remove") {
                                            pair status_msg = TA.change_territory_member(tty_name, "remove", args[2]);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "manager") {
                        try {
                            string player_name = sender.getName();
                            if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                                const string& tty_name = args[3];
                                Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (Territory_Action::check_tty_owner(tty_name, player_name) == true) {
                                        if (args[1] == "add") {
                                            pair status_msg = TA.change_territory_manager(tty_name, "add", args[2]);
                                            if (status_msg.first) {
                                                sender.sendMessage(status_msg.second);
                                            } else {
                                                sender.sendErrorMessage(status_msg.second);
                                            }
                                        } else if (args[1] == "remove") {
                                            pair status_msg = TA.change_territory_manager(tty_name, "remove", args[2]);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "settp") {
                        try {
                            if (!args[1].empty() && !args[2].empty()) {
                                string player_name = sender.getName();
                                const string& tty_name = args[2];
                                string dim = getServer().getPlayer(
                                        player_name)->getLocation().getDimension()->getName();
                                if (Territory_Action::check_tty_op(tty_name, player_name) == true) {
                                    Territory_Action::Point3D tp_pos = Territory_Action::pos_to_tuple(args[1]);
                                    pair status_msg = TA.change_tty_tppos(tty_name, tp_pos, dim);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "transfer") {
                        try {
                            string player_name = sender.getName();
                            if (!args[1].empty() && !args[2].empty()) {
                                const string& tty_name = args[1];
                                Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                                if (tty_data == nullptr) {
                                    sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                } else {
                                    if (Territory_Action::check_tty_owner(tty_name, player_name) == true) {
                                        pair status_msg = TA.change_territory_owner(tty_name, player_name, args[2]);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "tp") {
                        try {
                            if (!args[1].empty()) {
                                string player_name = sender.getName();
                                const string& tty_name = args[1];
                                Territory_Action::TerritoryData *tty_info = Territory_Action::read_territory_by_name(tty_name);
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
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    } else if (args[0] == "quick") {
                        try {
                            if (args[1] == "add") {
                                if (quick_create_player_data.contains(sender.getName())) {
                                    quick_create_player_data.erase(sender.getName());
                                }
                                auto tmp = quick_create_player_data[sender.getName()];
                                tmp.tty_type = "tty";
                                tmp.player_name = sender.getName();
                                quick_create_player_data[sender.getName()] = tmp;
                                sender.sendMessage(LangTty.getLocal("进入快速创建领地模式，请使用木棍点地确定领地第一个点"));
                            } else {
                                if (quick_create_player_data.contains(sender.getName())) {
                                    quick_create_player_data.erase(sender.getName());
                                }
                                auto tmp = quick_create_player_data[sender.getName()];
                                tmp.tty_type = "sub_tty";
                                tmp.player_name = sender.getName();
                                quick_create_player_data[sender.getName()] = tmp;
                                sender.sendMessage(LangTty.getLocal("进入快速创建子领地模式，请使用木棍点地确定子领地第一个点"));
                            }
                        } catch (const std::exception &e) {
                            sender.sendErrorMessage(e.what());
                        }
                    }else if (args[0] == "help") {
                        try {
                            string player_name = sender.getName();
                            string help_info = LangTty.getLocal("新建领地--/tty add 领地边角坐标1 领地边角坐标2\n新建子领地--/tty add_sub 子领地边角坐标1 子领地边角坐标2\n快速创建领地--/tty quick add\n快速创建子领地--/tty quick add_sub\n列出领地--/tty list\n删除领地--/tty del 领地名\n重命名领地--/tty rename 旧领地名 新领地名\n设置领地权限--/tty set 权限名(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage) 权限值 领地名\n设置领地管理员--/tty manager add|remove(添加|删除) 玩家名 领地名\n设置领地成员--/tty member add|remove(添加|删除) 玩家名 领地名\n设置领地传送点--/tty settp 领地传送坐标 领地名\n传送领地--/tty tp 领地名\n");
                            sender.sendMessage(help_info);
                        } catch (const std::exception &e) {
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
                    if (!args[1].empty()) {
                        if (Territory_Action::read_territory_by_name(args[1]) != nullptr) {
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
                    if (!args[1].empty()) {
                        auto tty_info = Territory_Action::list_player_tty(args[1]);
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
                    (void)readAllTerritories();
                    json json_msg = read_config();
                    try {
                        if (!json_msg.contains("error")) {
                            max_tty_num = json_msg["player_max_tty_num"];
                            actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
                            max_tty_area = json_msg["max_tty_area"];
                            if (json_msg["money_with_umoney"]) {
                                if (json_msg["money_with_umoney"] && umoney_check_exists() && json_msg["price"] >0) {
                                    money_with_umoney = true;
                                    price = json_msg["price"];
                                } else {
                                    money_with_umoney = false;
                                    price = 1;
                                    getLogger().error(LangTty.getLocal("经济配置错误,检查umoney插件是否安装,或者价格是否大于0;领地价格依然保持默认"));
                                }
                            } else {
                                money_with_umoney = false;
                                price = 1;
                            }
                        } else {
                            getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
                            max_tty_num = 20;
                            actor_fire_attack_protect = true;
                            money_with_umoney = false;
                            price = 1;
                            max_tty_area = 4000000;
                        }
                    } catch (const std::exception& e) {
                        max_tty_num = 20;
                        actor_fire_attack_protect = true;
                        money_with_umoney = false;
                        price = 1;
                        max_tty_area = 4000000;
                        getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置")+","+e.what());
                    }
                    getLogger().info(LangTty.getLocal("重载领地配置和数据完成"));
                } else if (args[0] == "set") {
                    try {
                        string player_name = sender.getName();
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                            const string& tty_name = args[3];
                            Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                            if (tty_data == nullptr) {
                                getLogger().error(LangTty.getLocal("未知的领地"));
                            } else {
                                int per_val;
                                if (args[2] == "true") {
                                    per_val = 1;
                                } else {
                                    per_val = 0;
                                }
                                pair status_msg = TA.change_territory_permissions(tty_name, args[1], per_val);
                                if (status_msg.first) {
                                    getLogger().info(status_msg.second);
                                } else {
                                    getLogger().error(status_msg.second);
                                }
                            }
                        } else {
                            getLogger().error(LangTty.getLocal("缺少参数"));
                        }
                    } catch (const std::exception &e) {
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
                    if (!args[1].empty()) {
                        if (Territory_Action::read_territory_by_name(args[1]) != nullptr) {
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
                    if (!args[1].empty()) {
                        auto tty_info = Territory_Action::list_player_tty(args[1]);
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
                    (void)readAllTerritories();
                    json json_msg = read_config();
                    try {
                        if (!json_msg.contains("error")) {
                            max_tty_num = json_msg["player_max_tty_num"];
                            actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
                            max_tty_area = json_msg["max_tty_area"];
                            if (json_msg["money_with_umoney"]) {
                                if (json_msg["money_with_umoney"] && umoney_check_exists() && json_msg["price"] >0) {
                                    money_with_umoney = true;
                                    price = json_msg["price"];
                                } else {
                                    money_with_umoney = false;
                                    price = 1;
                                    sender.sendErrorMessage(LangTty.getLocal("经济配置错误,检查umoney插件是否安装,或者价格是否大于0;领地价格依然保持默认"));
                                }
                            } else {
                                money_with_umoney = false;
                                price = 1;
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("配置文件错误,使用默认配置"));
                            max_tty_num = 20;
                            actor_fire_attack_protect = true;
                            money_with_umoney = false;
                            price = 1;
                            max_tty_area = 4000000;
                        }
                    } catch (const std::exception& e) {
                        max_tty_num = 20;
                        actor_fire_attack_protect = true;
                        money_with_umoney = false;
                        price = 1;
                        max_tty_area = 4000000;
                        sender.sendErrorMessage(LangTty.getLocal("配置文件错误,使用默认配置")+","+e.what());
                    }
                    sender.sendMessage(LangTty.getLocal("重载领地配置和数据完成"));
                } else if (args[0] == "set") {
                    try {
                        string player_name = sender.getName();
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                            const string& tty_name = args[3];
                            Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name);
                            if (tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                int per_val;
                                if (args[2] == "true") {
                                    per_val = 1;
                                } else {
                                    per_val = 0;
                                }
                                pair status_msg = TA.change_territory_permissions(tty_name, args[1], per_val);
                                if (status_msg.first) {
                                    sender.sendMessage(status_msg.second);
                                } else {
                                    sender.sendErrorMessage(status_msg.second);
                                }
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                }
            }
        }
        return true;
}

// 事件监听
    //方块破坏监听
    static void onBlockBreak(endstone::BlockBreakEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Territory_Action::Point3D block_pos = {event.getBlock().getLocation().getBlockX(),event.getBlock().getLocation().getBlockY(),event.getBlock().getLocation().getBlockZ()};
        auto player_in_tty = Territory_Action::list_in_tty(block_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_break)) {
                    if (ranges::find(info.members,player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上破坏的权限"));
                        return;
                    }
                }
            }
        }
    }

    //方块放置监听
    static void onBlockPlace(endstone::BlockPlaceEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Territory_Action::Point3D block_pos = {event.getBlock().getLocation().getBlockX(),event.getBlock().getLocation().getBlockY(),event.getBlock().getLocation().getBlockZ()};
        auto player_in_tty = Territory_Action::list_in_tty(block_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_build)) {
                    if (ranges::find(info.members,player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上放置的权限"));
                        return;
                    }
                }
            }
        }
    }

    //玩家交互监听
    static void onPlayerjiaohu(endstone::PlayerInteractEvent& event)
    {
        if (!event.getBlock()) {
            return;
        }
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Territory_Action::Point3D block_pos = {event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
        auto player_in_tty = Territory_Action::list_in_tty(block_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_jiaohu)) {
                    if (ranges::find(info.members,player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上交互的权限"));
                        return;
                    }
                }
            }
        }
    }

    //玩家实体交互监听
    static void onPlayerjiaohust(endstone::PlayerInteractActorEvent& event)
    {
        string player_name = event.getPlayer().getName();
        string player_dim = event.getPlayer().getLocation().getDimension()->getName();
        Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
        auto player_in_tty = Territory_Action::list_in_tty(actor_pos,player_dim);
        //检查玩家是否在领地上
        if (player_in_tty != std::nullopt) {
            for (const auto&info : player_in_tty.value()) {
                if (!(info.if_jiaohu)) {
                    if (ranges::find(info.members,player_name) == info.members.end()) {
                        event.setCancelled(true);
                        event.getPlayer().sendErrorMessage(LangTty.getLocal("你没有在此领地上交互的权限"));
                        return;
                    }
                }
            }
        }
    }

    //实体爆炸监听
    static void onActorBomb(endstone::ActorExplodeEvent& event)
    {
        string actor_dim = event.getActor().getLocation().getDimension()->getName();
        Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
        auto actor_in_tty = Territory_Action::list_in_tty(actor_pos,actor_dim);
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
    static void onActorhit(endstone::ActorDamageEvent& event)
    {
        string actor_dim = event.getActor().getLocation().getDimension()->getName();
        Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
        auto actor_in_tty = Territory_Action::list_in_tty(actor_pos,actor_dim);
        //检查实体是否在领地上
        if (actor_in_tty != std::nullopt) {
            for (const auto&info : actor_in_tty.value()) {
                if (!(info.if_damage)) {
                    auto actor_or_player = event.getDamageSource().getActor();
                    // 玩家导致伤害
                    if (actor_or_player && actor_or_player->getType() == "minecraft:player") {
                        if (ranges::find(info.members,actor_or_player->getName()) == info.members.end()) {
                            actor_or_player->sendErrorMessage(LangTty.getLocal("你没有在此领地上伤害的权限"));
                            event.setCancelled(true);
                        }
                        //火焰攻击类伤害
                    } else if (event.getDamageSource().getType() == "fire_tick") {
                        //非玩家实体才免疫
                        if (event.getActor().getType() != "minecraft:player") {
                            if (actor_fire_attack_protect) {
                                event.setCancelled(true);
                            }
                        }
                    }
                    return;
                }
            }
        }
    }

    //快速创建领地-右键事件
    static void quickCreateTtyRightClick(const endstone::PlayerInteractEvent& event) {
        if (!quick_create_player_data.contains(event.getPlayer().getName())) {
            return;
        }
        if (!event.getItem()) {
            return;
        }
        if (event.getItem()->getType().getId() == "minecraft:stick") {
            if (!event.getBlock()) {
                return;
            }
            //在玩家处于快速创建模式且手持木棍点击非空气时开始提示
            auto& player = event.getPlayer();
            auto tmp = quick_create_player_data[player.getName()];
            if (tmp.dim1.empty()) {
                tmp.dim1 = player.getDimension().getName();
                tmp.pos1 = Territory_Action::Point3D{event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
                player.sendMessage(LangTty.getLocal("已记录此坐标为第一个坐标，请选择第二个坐标"));
            } else if (!tmp.dim2.empty()){
                //点二已存在数据，去重
                return;
            } else {
                tmp.dim2 = player.getDimension().getName();
                //进行维度检查
                if (tmp.dim1 != tmp.dim2) {
                    player.sendErrorMessage(LangTty.getLocal("坐标在不同维度，无法创建领地"));
                    return;
                }
                tmp.pos2 = Territory_Action::Point3D{event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
                //由于交互事件可能多次触发，检查点重复
                if (tmp.pos1 == tmp.pos2) {
                    return;
                }
                player.sendMessage(LangTty.getLocal("已记录此坐标为第二个坐标，请通过提示完成领地创建"));
            }
            //存储坐标数据
            quick_create_player_data[player.getName()] = tmp;
            if (!tmp.dim2.empty()) {
                Menu::openQuickCreateTtyMenu(&player,tmp);
            }
        }
    }
private:
    std::unique_ptr<Menu> menu_;
};


#endif //TERRITORY_TERRITORYCORE_H