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
    const std::string umoney_file = "plugins/umoney/money.json";
    const std::string language_path = "plugins/territory/language/";

    //一些全局变量

    int config_max_tty_num;
    bool config_actor_fire_attack_protect;
    bool config_money_with_umoney;
    int config_price;
    int config_max_tty_area;
    bool config_welcome_all;
    string language = "en_US";
    vector<int64_t> config_entity_can_die;
    bool config_fly_on_tty;

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

    // 从数据库读取所有领地数据
    static void readAllTerritories();

    // 提示领地信息函数
    //void tips_online_players() const;

    //接入umoney
    //检查插件存在
    [[nodiscard]] bool umoney_check_exists() const;
    
    //获取玩家资金
    [[nodiscard]] int umoney_get_player_money(const std::string& player_name) const;
    
    //更改玩家资金
    [[nodiscard]] bool umoney_change_player_money(const std::string& player_name, int money) const;

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
};


#endif //TERRITORY_TERRITORYCORE_H