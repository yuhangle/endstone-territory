//
// Created by yuhang on 2025/9/3.
//

#ifndef TERRITORY_TERRITORYCORE_H
#define TERRITORY_TERRITORYCORE_H

#include "database.hpp"
#include <endstone/endstone.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <tuple>
#include <vector>
#include "territory_action.h"
#include "menu.h"
#include "translate.hpp"
#include "event_listener.h"
#include <money_connect/money_connect.h>

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

class Territory : public endstone::Plugin {
public:
    friend class EventListener;
    //数据文件路径
    std::string data_path = "plugins/territory";
    std::string config_path = "plugins/territory/config.json";
    const std::string db_file = "plugins/territory/territory_data.db";
    const std::string language_path = "plugins/territory/language/";

    //一些变量
    int config_max_tty_num;
    bool config_actor_fire_attack_protect;
    bool config_money_connect;
    double config_price;
    int config_max_tty_area;
    bool config_welcome_all;
    string language = "en_US";
    vector<int64_t> config_entity_can_die;
    bool config_fly_on_tty;

    //禁止进入领地的实体
    const std::unordered_set<std::string> no_allow_entitys = {"minecraft:wither"};

    //翻译类
    translate LangTty;
    //全局玩家位置信息
    std::unordered_map<std::string, std::tuple<Territory_Action::Point3D, string, string>> lastPlayerPositions;

    //快速创建领地玩家数据缓存
    std::unordered_map<std::string,Territory_Action::QuickTtyData> quick_create_player_data;

    // 读取配置文件
    [[nodiscard]] json read_config() const;

    //数据目录和配置文件检查
    void datafile_check();

    //接入money_connect经济桥接
    //尝试查找经济服务（懒加载）
    void try_find_economy();
    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const;

    //获取玩家资金
    [[nodiscard]] double get_player_money(const std::string& player_name) const;

    //更改玩家资金
    [[nodiscard]] bool change_player_money(const std::string& player_name, double money) const;

    //实体位置监听
    void entity_move_listener() const;

    //插件初始化
    static Territory& getInstance();
    void onLoad() override;
    void onEnable() override;
    void onDisable() override;

    bool onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) override;

private:
    static Territory* instance_;
    std::unique_ptr<Menu> menu_;
    std::unique_ptr<DataBase> database_;
    std::unique_ptr<Territory_Action> action_;
    std::unique_ptr<EventListener> event_listener_;
    std::shared_ptr<money_connect::EconomyService> economy_service_;
    std::shared_ptr<endstone::Task> lazy_find_economy_;
};


#endif //TERRITORY_TERRITORYCORE_H