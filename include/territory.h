//
// Created by yuhang on 2025/9/3.
//

#ifndef TERRITORY_TERRITORYCORE_H
#define TERRITORY_TERRITORYCORE_H

#include "database.hpp"
#include <endstone/endstone.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <filesystem>
#include <tuple>
#include <vector>
#include <random>
#include "territory_action.h"
#include "menu.h"
#include "translate.hpp"

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

//数据文件路径
inline std::string data_path = "plugins/territory";
inline std::string config_path = "plugins/territory/config.json";
inline const std::string db_file = "plugins/territory/territory_data.db";
inline const std::string umoney_file = "plugins/umoney/money.json";
inline const std::string language_path = "plugins/territory/language/";

//一些全局变量
inline int max_tty_num;
inline bool actor_fire_attack_protect;
inline bool money_with_umoney;
inline int price;
inline int max_tty_area;
inline bool welcome_all;
inline string language = "en_US";
inline vector<int64_t> entity_can_die;

//初始化其它实例
extern DataBase Database;
extern Territory_Action TA;
//翻译类
extern translate LangTty;
//全局玩家位置信息
extern std::unordered_map<std::string, std::tuple<Territory_Action::Point3D, string, string>> lastPlayerPositions;

//快速创建领地玩家数据缓存
extern std::unordered_map<std::string,Territory_Action::QuickTtyData> quick_create_player_data;

class Territory : public endstone::Plugin {
public:
    // 读取配置文件
    [[nodiscard]] json read_config() const;

    //数据目录和配置文件检查
    void datafile_check() const;

    // 从数据库读取所有领地数据
    static void readAllTerritories();

    // 提示领地信息函数
    void tips_online_players() const;

    // 删除玩家领地函数，删除名称为 tty_name 的领地，并更新相关数据
    [[nodiscard]] bool del_player_tty(const std::string &tty_name) const;

    //接入umoney
    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const;
    
    //获取玩家资金
    static int umoney_get_player_money(const std::string& player_name);
    
    //更改玩家资金
    [[nodiscard]] bool umoney_change_player_money(const std::string& player_name, int money) const;

    //插件初始化
    void onLoad() override;
    void onEnable() override;
    void onDisable() override;

    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) override;

    // 事件监听
    //方块破坏监听
    static void onBlockBreak(endstone::BlockBreakEvent& event);

    //方块放置监听
    static void onBlockPlace(endstone::BlockPlaceEvent& event);

    //玩家交互监听
    static void onPlayerjiaohu(endstone::PlayerInteractEvent& event);

    //玩家实体交互监听
    static void onPlayerjiaohust(endstone::PlayerInteractActorEvent& event);

    //实体爆炸监听
    static void onActorBomb(endstone::ActorExplodeEvent& event);

    //实体受击
    static void onActorhit(endstone::ActorDamageEvent& event);

    //实体死亡
    static void onActorDeath(const endstone::ActorDeathEvent& event);

    //快速创建领地-右键事件
    static void quickCreateTtyRightClick(const endstone::PlayerInteractEvent& event);

private:
    std::unique_ptr<Menu> menu_;
};


#endif //TERRITORY_TERRITORYCORE_H