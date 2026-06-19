//
// Created by yuhang on 2025/3/6.
//

#include "territory.h"
#include "translate.hpp"
#include "version.h"
#include "event_listener.h"

//插件信息
ENDSTONE_PLUGIN("territory", TERRITORY_PLUGIN_VERSION, Territory)
{
    description = "a territory plugin for endstone with C++";
    website = "https://github.com/yuhangle/endstone-territory";
    authors = {"yuhang2006 <yuhang2006@hotmail.com>"};

    command("tty")
            .description("Territory command")
            .usages("/tty (add)[opt: opt_add] [pos: pos] [pos: pos]",
                    "/tty (add_sub)[opt: opt_addsub] [pos: pos] [pos: pos]",
                    "/tty (quick)<opt: opt_quick> (add|add_sub)<opt: opt_qa>",
                    "/tty (list)[opt: opt_list]",
                    "/tty (del)[opt: opt_del] [territory: str]",
                    "/tty (rename)[opt: opt_rename] [old_name: str] [new_name: str]",
                    "/tty (set)<opt: opt_setper> (if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage|if_edge_piston|if_wither)<opt: opt_permission> <bool: bool> <territory: str>",
                    "/tty (member)<opt: opt_member> (add|remove)<opt: opt_mem> <player: target> <territory: str>",
                    "/tty (manager)<opt: opt_manager> (add|remove)<opt: opt_man> <player: target> <territory: str>",
                    "/tty (settp)<opt: opt_settp> [pos: pos] <territory: str>",
                    "/tty (transfer)<opt: opt_transfer> <territory: str> <player: target>",
                    "/tty (tp)<opt: opt_tp> <territory: str>",
                    "/tty (resize)<opt: resize> <territory: str> [pos: pos] [pos: pos]",
                    "/tty (help)<opt: opt_help>"
                    )
            .permissions("territory.command.member");

    command("optty")
            .description("Territory op command")
            .usages("/optty (del)[opt: opt_op_del] [msg: message]",
                    "/optty (del_all)[opt: opt_op_del_all] <player: target>",
                    "/optty <transfer> <territory: str> <player: target>",
                    "/optty <transfer_all> <player: target> <player: target>",
                    "/optty (set)<opt: opt_op_setper> (if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage|if_edge_piston|if_wither)<opt: opt_op_permission> <bool: bool> <msg: message>",
                    "/optty (reload)[opt: opt_reloadtty]"
            )
            .permissions("territory.command.op");

    permission("territory.command.member")
            .description("member command")
            .default_(endstone::PermissionDefault::True);

    permission("territory.command.op")
            .description("op command")
            .default_(endstone::PermissionDefault::Operator);
}


// 初始化静态成员
Territory* Territory::instance_ = nullptr;

Territory& Territory::getInstance() {
    return *instance_;
}

//全局玩家位置信息
std::unordered_map<std::string, std::tuple<Territory_Action::Point3D, string, string>> lastPlayerPositions;

//快速创建领地玩家数据缓存
std::unordered_map<std::string,Territory_Action::QuickTtyData> quick_create_player_data;

// 读取配置文件
json Territory::read_config() const {
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
void Territory::datafile_check() {
    json df_config = {
        {"player_max_tty_num", 20},
        {"actor_fire_attack_protect", true},
        {"money_connect", false},
        {"price", 1},
        {"max_tty_area",4000000},
        {"welcome_all",true},
        {"language","zh_CN"},
        {"allow_fly_on_territory", false},
        {"allow_op_as_member", false}
    };

    if (!(std::filesystem::exists(data_path))) {
        getLogger().info(LangTty.getLocal("Territory数据目录未找到,已自动创建"));
        std::filesystem::create_directory("plugins/territory");
        if (!(std::filesystem::exists("plugins/territory/config.json"))) {
            if (std::ofstream file(config_path); file.is_open()) {
                file << df_config.dump(4);
                file.close();
                getLogger().info(LangTty.getLocal("配置文件已创建"));
            }
        }
    } else if (std::filesystem::exists(data_path)) {
        if (!(std::filesystem::exists("plugins/territory/config.json"))) {
            if (std::ofstream file(config_path); file.is_open()) {
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
                    getLogger().info(LangTty.tr("配置项 '{}' 已根据默认配置进行更新",key));
                    need_update = true;
                }
            }

            // 如果需要更新配置文件，则进行写入
            if (need_update) {
                if (std::ofstream outfile(config_path); outfile.is_open()) {
                    outfile << loaded_config.dump(4);
                    outfile.close();
                    getLogger().info(LangTty.getLocal("配置文件已完成更新"));
                }
            }
        }
    }
    if (!filesystem::exists(language_path))
    {
        std::filesystem::create_directory(language_path);
    }
}

//尝试查找经济服务（懒加载）
void Territory::try_find_economy() {
    if (economy_service_) return; // 已找到
    if (const auto plugin = getServer().getPluginManager().getPlugin("money_connect"); !plugin) return; // 插件还未加载
    economy_service_ = getServer().getServiceManager().load<money_connect::EconomyService>("MoneyConnect");
}

//检查插件存在（经济服务是否可用）
bool Territory::umoney_check_exists() const {
    return economy_service_ != nullptr;
}

//获取玩家资金
double Territory::get_player_money(const std::string& player_name) const
{
    if (!economy_service_) {
        getLogger().error("MoneyConnect economy service not available");
        return -1;
    }
    return economy_service_->get_balance(player_name);
}

//更改玩家资金
bool Territory::change_player_money(const std::string& player_name, const double money) const {
    if (!economy_service_) {
        getLogger().error("MoneyConnect economy service not available");
        return false;
    }
    if (money > 0) {
        economy_service_->add_balance(player_name, money);
    } else if (money < 0) {
        economy_service_->remove_balance(player_name, -money);
    }
    return true;
}

void Territory::entity_move_listener() const
{
    const auto actors = getServer().getLevel()->getActors();
    if (actors.empty()) return;

    const auto& checker = event_listener_->global_checker_;

    for (auto& actor : actors) {
        if (!no_allow_entitys.contains(actor->getType())) {
            continue;
        }

        const auto& loc = actor->getLocation();
        const string actor_dim = loc.getDimension().getName();
        const TerritoryInstance::Point3D actor_pos = {
            static_cast<double>(loc.getBlockX()),
            static_cast<double>(loc.getBlockY()),
            static_cast<double>(loc.getBlockZ())
        };

        if (const auto result = checker->canWitherExist(actor_pos, actor_dim);
            result.has_value() && !result.value()) {
            actor->remove();
            }
    }
}

//插件初始化
void Territory::onLoad()
{
    getLogger().info("onLoad is called");
    instance_ = this;
    //检查文件
    datafile_check();
    //初始化各个类
    database_ = std::make_unique<DataBase>(db_file);
    action_ = std::make_unique<Territory_Action>(*database_, this, LangTty);
    (void)database_->init_database();
    // translate 构造函数已自动加载所有语言文件
}

void Territory::onEnable()
{
    getLogger().info("onEnable is called");

    json json_msg = read_config();
    //默认配置
    config_max_tty_num = 20;
    config_actor_fire_attack_protect = true;
    config_money_connect = false;
    config_price = 1.0;
    config_max_tty_area = 4000000;
    config_welcome_all = true;
    config_fly_on_tty = false;
    config_allow_op_as_member = false;
    try {
        if (!json_msg.contains("error")) {
            config_max_tty_num = json_msg["player_max_tty_num"];
            config_actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
            config_max_tty_area = json_msg["max_tty_area"];
            config_welcome_all = json_msg["welcome_all"];
            config_fly_on_tty = json_msg["allow_fly_on_territory"];
            config_allow_op_as_member = json_msg["allow_op_as_member"];
            language = json_msg["language"];
            if (json_msg["money_connect"]) {
                if (json_msg["price"] > 0) {
                    config_money_connect = true;
                    config_price = json_msg["price"];
                } else {
                    getLogger().error(LangTty.getLocal("经济配置错误,检查价格是否大于0;领地价格依然保持默认"));
                }
            }
        } else {
            getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置"));
        }
    } catch (const std::exception& e) {
        getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置")+","+e.what());
    }
    LangTty = translate(language_path, language);
    menu_ = std::make_unique<Menu>(*this, LangTty);
    event_listener_ = std::make_unique<EventListener>(this, LangTty);
    //注册事件监听
    registerEvent<endstone::BlockBreakEvent>([this](auto& e) {event_listener_->onBlockBreak(e);});
    registerEvent<endstone::BlockPlaceEvent>([this](auto& e) { event_listener_->onBlockPlace(e); });
    registerEvent<endstone::PlayerInteractEvent>([this](auto& e) { event_listener_->onPlayerjiaohu(e); });
    registerEvent<endstone::PlayerInteractActorEvent>([this](auto& e) { event_listener_->onPlayerjiaohust(e); });
    registerEvent<endstone::ActorDamageEvent>([this](auto& e) { event_listener_->onActorhit(e); });
    registerEvent<endstone::ActorDeathEvent>([this](auto& e) { event_listener_->onActorDeath(e); });
    registerEvent<endstone::BlockPistonExtendEvent>([this](auto& e) {event_listener_->onEdgePiston(e);});
    registerEvent<endstone::BlockPistonRetractEvent>([this](auto& e) {event_listener_->onEdgePiston(e);});
    registerEvent<endstone::ActorExplodeEvent>([this](auto& e) { event_listener_->onActorBomb(e); });
    registerEvent<endstone::ActorSpawnEvent>([this](auto& e) { event_listener_->onEnititySummon(e); });
    //快速创建领地选择监听
    registerEvent<endstone::PlayerInteractEvent>([this](auto& e){ event_listener_->quickCreateTtyRightClick(e); });
    //玩家移动监听
    registerEvent<endstone::PlayerMoveEvent>([this](auto& e) { event_listener_->onPlayerMove(e); });
    //数据库读取
    (void)action_->get_all_tty();
    //周期执行
    //getServer().getScheduler().runTaskTimer(*this,[&]() { tips_online_players(); }, 0, 25);
    //实体移动监听
    getServer().getScheduler().runTaskTimer(*this,[&]() { entity_move_listener(); }, 0, 20);

    // 懒加载经济服务：每隔20tick尝试查找money_connect，找到后取消任务
    if (config_money_connect) {
        lazy_find_economy_ = getServer().getScheduler().runTaskTimer(*this, [this]() {
            try_find_economy();
            if (economy_service_) {
                getLogger().info("MoneyConnect economy service loaded successfully");
                lazy_find_economy_->cancel();
                lazy_find_economy_ = nullptr;
            }
        }, 0, 20);
    }

    //显示启动信息
    const string boot_logo_msg = R"(
___________                 .__  __
\__    ___/_________________|__|/  |_  ___________ ___.__.
  |    |_/ __ \_  __ \_  __ \  \   __\/  _ \_  __ <   |  |
  |    |\  ___/|  | \/|  | \/  ||  | (  <_> )  | \/\___  |
  |____| \___  >__|   |__|  |__||__|  \____/|__|   / ____|
             \/                                    \/\
)";

    getLogger().info(boot_logo_msg);
    getLogger().info("Language system initialized, default: " + language);
    const auto enable_msg = endstone::ColorFormat::Yellow + LangTty.getLocal("Territory插件已启用, Version: ") + getServer().getPluginManager().getPlugin("territory")->getDescription().getVersion();
    getLogger().info(enable_msg);
    getLogger().info(endstone::ColorFormat::Yellow + "Project address: https://github.com/yuhangle/endstone-territory");
}

void Territory::onDisable()
{
    getLogger().info("onDisable is called");
    getServer().getScheduler().cancelTasks(*this);
    Territory_Action::clearCache();
}

bool Territory::onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) {
    // 根据发送者 locale 确定翻译语言（玩家用其客户端语言，控制台用配置的默认语言）
    string localeP = language;
    if (sender.asPlayer()) localeP = sender.asPlayer()->getLocale();

    if (command.getName() == "tty")
        if (sender.asPlayer() == nullptr) {
            getLogger().error(LangTty.getLocal("无法在服务端使用tty命令!", localeP));
        } else {
            if (args.empty()) {
                //菜单
                if (getServer().getPluginManager().getPlugin("territory_gui")) {
                    sender.sendMessage(LangTty.getLocal("Territory_gui插件已不再需要，菜单已被集成到插件本体内", localeP));
                    menu_->openMainMenu(sender.asPlayer());
                } else {
                    menu_->openMainMenu(sender.asPlayer());
                }
            } else {
                if (args[0] == "add") {
                    try
                    {
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
                            sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地", localeP));
                            return false;
                        }
                        //维度
                        string dim = getServer().getPlayer(player_name)->getLocation().getDimension().getName();
                        
                        // 检查领地大小
                        const int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
                        if (area >= config_max_tty_area && config_max_tty_area != -1) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建", localeP));
                            return false;
                        }
                        
                        // 检查玩家领地数量是否达到上限
                        if (Territory_Action::check_tty_num(player_name) >= config_max_tty_num) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地", localeP));
                            return false;
                        }
                        // 检查资金
                        if (config_money_connect) {
                            const double money = get_player_money(player_name);
                            if (const auto value = area * config_price; money >= value) {
                                (void)change_player_money(player_name,-value);
                                sender.sendMessage(LangTty.getLocal("设置领地已扣费:", localeP) + to_string(value));
                                // 调用领地创建函数
                                if (auto [success, message] = action_->create_territory(player_name, pos1, pos2, tppos, dim); success) {
                                    sender.sendMessage(LangTty.getLocal("成功添加领地", localeP));
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal(message, localeP));
                                    (void)change_player_money(player_name,value); // 创建失败返还资金
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("你的资产不足以设置此大小的领地,设置此领地所需要的资金为:", localeP) +
                                                                             to_string(value));
                                return false;
                            }
                        }
                        // 不使用经济
                        else {
                            // 调用领地创建函数
                            if (auto [success, message] = action_->create_territory(player_name, pos1, pos2, tppos, dim); success) {
                                sender.sendMessage(LangTty.getLocal("成功添加领地", localeP));
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal(message, localeP));
                            }
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "add_sub") {
                    try
                    {
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
                            sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地", localeP));
                            return false;
                        }
                        //维度
                        string dim = getServer().getPlayer(player_name)->getLocation().getDimension().getName();
                        
                        // 检查领地大小
                        const int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
                        if (area >= config_max_tty_area && config_max_tty_area != -1) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建", localeP));
                            return false;
                        }
                        
                        // 检查玩家领地数量是否达到上限
                        if (Territory_Action::check_tty_num(player_name) >= config_max_tty_num) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地", localeP));
                            return false;
                        }

                        // 检查资金
                        if (config_money_connect) {
                            const double money = get_player_money(player_name);
                            const auto value = area * config_price;
                            if (money >= value) {
                                (void)change_player_money(player_name,-value);
                                sender.sendMessage(LangTty.getLocal("设置领地已扣费:", localeP) + to_string(value));
                                // 调用子领地创建函数
                                if (auto [success, child_territory_name] = action_->create_sub_territory(player_name, pos1, pos2, tppos, dim); success) {
                                    sender.sendMessage(LangTty.getLocal("成功添加子领地,归属于父领地: ", localeP) + child_territory_name);

                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal(child_territory_name, localeP)); // child_territory_name here contains error message
                                    (void)change_player_money(player_name,value); // 创建失败返还资金
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("你的资产不足以设置此大小的领地,设置此领地所需要的资金为:", localeP) +
                                                                             to_string(value));
                                return false;
                            }
                        }
                        else {
                            // 调用子领地创建函数
                            if (auto [success, child_territory_name] = action_->create_sub_territory(player_name, pos1, pos2, tppos, dim); success) {
                                sender.sendMessage(LangTty.getLocal("成功添加子领地,归属于父领地: ", localeP) + child_territory_name);
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal(child_territory_name, localeP)); // child_territory_name here contains error message
                            }
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "list") {
                    try {
                        menu_->openListTtyMenu(sender.asPlayer());
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "del") {
                    try {
                        string player_name = sender.getName();
                        if (!args[1].empty()) {
                            if (const auto tty_data = Territory_Action::read_territory_by_name(args[1]); tty_data != nullptr) {
                                if (Territory_Action::is_tty_owner(tty_data->name, player_name) == true) {
                                    if (action_->del_player_tty(tty_data->name)) {
                                        sender.sendMessage(LangTty.getLocal("已成功删除领地", localeP));
                                    } else {
                                        sender.sendErrorMessage(LangTty.getLocal("删除领地失败", localeP));
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限删除此领地", localeP));
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                            }


                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "rename") {
                    try {
                        string player_name = sender.getName();
                        const string& tty_name = args[1];
                        if (TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                            sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                        } else {
                            if (!args[1].empty() && !args[2].empty()) {
                                if (Territory_Action::is_tty_owner(args[1], player_name) == true) {
                                    if (auto [fst, snd] = action_->rename_player_tty(args[1], args[2]); fst) {
                                        sender.sendMessage(snd);
                                    } else {
                                        sender.sendErrorMessage(snd);
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限重命名此领地", localeP));
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
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
                            if (TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                            } else {
                                if (Territory_Action::is_tty_op(tty_name, player_name) == true) {
                                    int per_val;
                                    if (args[2] == "true") {
                                        per_val = 1;
                                    } else {
                                        per_val = 0;
                                    }
                                    if (auto [fst, snd] = action_->change_territory_permissions(tty_name, args[1], per_val); fst) {
                                        sender.sendMessage(snd);
                                    } else {
                                        sender.sendErrorMessage(snd);
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地", localeP));
                                }
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "member") {
                    try {
                        string player_name = sender.getName();
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                            const string& tty_name = args[3];
                            if (TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                            } else {
                                if (Territory_Action::is_tty_op(tty_name, player_name) == true) {
                                    if (args[1] == "add") {
                                        if (auto [fst, snd] = action_->change_territory_member(tty_name, "add", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
                                        }
                                    } else if (args[1] == "remove") {
                                        if (auto [fst, snd] = action_->change_territory_member(tty_name, "remove", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
                                        }
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地", localeP));
                                }
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "manager") {
                    try {
                        string player_name = sender.getName();
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                            const string& tty_name = args[3];
                            if (TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                            } else {
                                if (Territory_Action::is_tty_owner(tty_name, player_name) == true) {
                                    if (args[1] == "add") {
                                        if (auto [fst, snd] = action_->change_territory_manager(tty_name, "add", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
                                        }
                                    } else if (args[1] == "remove") {
                                        if (auto [fst, snd] = action_->change_territory_manager(tty_name, "remove", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
                                        }
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地", localeP));
                                }
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
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
                                    player_name)->getLocation().getDimension().getName();
                            if (Territory_Action::is_tty_op(tty_name, player_name) == true) {
                                Territory_Action::Point3D tp_pos = Territory_Action::pos_to_tuple(args[1]);
                                if (auto [fst, snd] = action_->change_tty_tppos(tty_name, tp_pos, dim, localeP); fst) {
                                    sender.sendMessage(snd);
                                } else {
                                    sender.sendErrorMessage(snd);
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地", localeP));
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "transfer") {
                    try {
                        string player_name = sender.getName();
                        if (!args[1].empty() && !args[2].empty()) {
                            const string& tty_name = args[1];
                            if (TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                            } else {
                                if (Territory_Action::is_tty_owner(tty_name, player_name) == true) {
                                    if (auto [fst, snd] = action_->change_territory_owner(tty_name, player_name, args[2]); fst) {
                                        sender.sendMessage(snd);
                                    } else {
                                        sender.sendErrorMessage(snd);
                                    }
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地", localeP));
                                }
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "tp") {
                    try {
                        if (!args[1].empty()) {
                            string player_name = sender.getName();
                            const string& tty_name = args[1];
                            if (TerritoryData *tty_info = Territory_Action::read_territory_by_name(tty_name); tty_info != nullptr) {
                                if (tty_info->if_tp) {
                                    auto player = getServer().getPlayer(player_name);
                                    auto tty_Dim = getServer().getLevel()->getDimension(tty_info->dim);
                                    endstone::Location loc(*tty_Dim, static_cast<float>(get<0>(tty_info->tppos)),
                                                           static_cast<float>(get<1>(tty_info->tppos)),
                                                           static_cast<float>(get<2>(tty_info->tppos)),
                                                           0.0, 0.0);

                                    player->teleport(loc);
                                    Menu::addRecentTp(player_name, tty_name);
                                } else if (tty_info->member.find(player_name) != std::string::npos ||
                                           tty_info->manager.find(player_name) != std::string::npos ||
                                           player_name == tty_info->owner) {
                                    auto player = getServer().getPlayer(player_name);
                                    auto tty_Dim = getServer().getLevel()->getDimension(tty_info->dim);
                                    endstone::Location loc(*tty_Dim, static_cast<float>(get<0>(tty_info->tppos)),
                                                           static_cast<float>(get<1>(tty_info->tppos)),
                                                           static_cast<float>(get<2>(tty_info->tppos)),
                                                           0.0, 0.0);
                                    player->teleport(loc);
                                    Menu::addRecentTp(player_name, tty_name);
                                } else if (config_allow_op_as_member && sender.asPlayer()->isOp())
                                {
                                    auto player = getServer().getPlayer(player_name);
                                    auto tty_Dim = getServer().getLevel()->getDimension(tty_info->dim);
                                    endstone::Location loc(*tty_Dim, static_cast<float>(get<0>(tty_info->tppos)),
                                                           static_cast<float>(get<1>(tty_info->tppos)),
                                                           static_cast<float>(get<2>(tty_info->tppos)),
                                                           0.0, 0.0);

                                    player->teleport(loc);
                                    Menu::addRecentTp(player_name, tty_name);
                                }
                                else {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有传送到此领地的权限", localeP));
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                            }
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
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
                            sender.sendMessage(LangTty.getLocal("进入快速创建领地模式，请使用木棍点地确定领地第一个点", localeP));
                        } else {
                            if (quick_create_player_data.contains(sender.getName())) {
                                quick_create_player_data.erase(sender.getName());
                            }
                            auto tmp = quick_create_player_data[sender.getName()];
                            tmp.tty_type = "sub_tty";
                            tmp.player_name = sender.getName();
                            quick_create_player_data[sender.getName()] = tmp;
                            sender.sendMessage(LangTty.getLocal("进入快速创建子领地模式，请使用木棍点地确定子领地第一个点", localeP));
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "resize") {
                    try
                    {
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty())
                        {
                            auto tty_data = Territory_Action::read_territory_by_name(args[1]);
                            if (tty_data == nullptr)
                            {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                                return false;
                            }
                            if (tty_data->owner != sender.getName())
                            {
                                sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地", localeP));
                                return false;
                            }
                            //领地位置1
                            Territory_Action::Point3D pos1 = Territory_Action::pos_to_tuple(args[2]);
                            //领地位置2
                            Territory_Action::Point3D pos2 = Territory_Action::pos_to_tuple(args[3]);
                            //传送坐标
                            Territory_Action::Point3D tppos = Territory_Action::pos_to_tuple(to_string(sender.asPlayer()->getLocation().getX()) + " " + to_string(sender.asPlayer()->getLocation().getY()) + " " + to_string(sender.asPlayer()->getLocation().getZ()));
                            if (get<1>(pos1) > 320 || get<1>(pos1) < -64 || get<1>(pos2) > 320 || get<1>(pos2) < -64) {
                                sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地", localeP));
                                return false;
                            }

                            // 检查领地大小
                            const int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
                            if (area >= config_max_tty_area && config_max_tty_area != -1) {
                                sender.sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建", localeP));
                                return false;
                            }
                            // 资金检查
                            double changed_money = 0;
                            if (config_money_connect)
                            {
                                const int old_area = Territory_Action::get_tty_area(static_cast<int>(get<0>(tty_data->pos1)),static_cast<int>(get<2>(tty_data->pos1)),static_cast<int>(get<0>(tty_data->pos2)),static_cast<int>(get<2>(tty_data->pos2)));
                                const auto old_value = old_area * config_price;
                                const auto new_value = area * config_price;
                                changed_money = new_value - old_value;
                                if (get_player_money(sender.getName()) < changed_money)
                                {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有足够的资金", localeP));
                                    return false;
                                }
                                if (changed_money != 0)
                                {
                                    (void)change_player_money(sender.getName(), -changed_money);
                                }
                            }

                            if (auto [fst, snd] = action_->resize_territory(pos1,pos2,*tty_data,tppos, localeP); fst)
                            {
                                sender.sendMessage(LangTty.getLocal(snd, localeP));
                            }  else
                            {
                                sender.sendErrorMessage(LangTty.getLocal(snd, localeP));
                                if (changed_money!=0)
                                {
                                    (void)change_player_money(sender.getName(), changed_money);
                                }
                            }

                        } else
                        {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                        }
                    } catch (const std::exception &e)
                    {
                        sender.sendErrorMessage(e.what());
                    }
                }
                else if (args[0] == "help") {
                    try {
                        sender.sendMessage(LangTty.getLocal("§l§2━━━ 领地帮助 ━━━\n\n§l§a创建类:\n  §r/tty add <坐标1> <坐标2>  新建领地\n  §r/tty add_sub <坐标1> <坐标2>  新建子领地\n  §r/tty quick add  快速创建领地\n  §r/tty quick add_sub  快速创建子领地\n\n§l§e管理类:\n  §r/tty set <权限> <值> <领地名>  修改权限\n  §r/tty member add/remove <玩家> <领地名>  管理成员\n  §r/tty manager add/remove <玩家> <领地名>  管理管理员\n  §r/tty settp <坐标> <领地名>  设置传送点\n  §r/tty del <领地名>  删除领地\n  §r/tty rename <旧名> <新名>  重命名领地\n  §r/tty resize <领地名>  更改领地大小\n\n§l§b查询类:\n  §r/tty list  列出我的领地\n  §r/tty tp <领地名>  传送至领地", localeP));
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                }
            }
        }
    else if (command.getName() == "optty") {
        if (args.empty()) {
            sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
            return false;
        }
        if (args[0] == "del") {
            if (!args[1].empty()) {
                if (Territory_Action::read_territory_by_name(args[1]) != nullptr) {
                    if (action_->del_player_tty(args[1])) {
                        sender.sendMessage(LangTty.getLocal("已删除领地", localeP));
                    } else {
                        sender.sendErrorMessage(LangTty.getLocal("领地删除失败", localeP));
                    }
                } else {
                    sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                }
            } else {
                sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
            }
        } else if (args[0] == "del_all") {
            if (!args[1].empty()) {
                if (auto tty_info = Territory_Action::list_player_tty(args[1]); tty_info.empty()) {
                    sender.sendErrorMessage(LangTty.getLocal("该玩家没有领地", localeP));
                } else {
                    sender.sendMessage(
                            LangTty.getLocal("正在删除玩家 ", localeP) + args[1] + LangTty.getLocal(" 的全部领地", localeP));
                    for (const auto &the_tty: tty_info) {
                        if (action_->del_player_tty(the_tty.name)) {
                            sender.sendMessage(LangTty.getLocal("成功删除领地: ", localeP) + the_tty.name);
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("删除领地失败: ", localeP) + the_tty.name);
                        }
                    }
                }
            } else {
                sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
            }
        } else if (args[0] == "reload") {
            (void)action_->get_all_tty();
            json json_msg = read_config();
            try {
                if (!json_msg.contains("error")) {
                    config_max_tty_num = json_msg["player_max_tty_num"];
                    config_actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
                    config_max_tty_area = json_msg["max_tty_area"];
                    config_welcome_all = json_msg["welcome_all"];
                    config_allow_op_as_member = json_msg["allow_op_as_member"];
                    if (json_msg["allow_fly_on_territory"] == false || config_fly_on_tty == true)
                    {
                        for (const auto pEntity : getServer().getOnlinePlayers())
                        {
                            if (static_cast<int>(pEntity->getGameMode()) == 0 || static_cast<int>(pEntity->getGameMode()) == 3)
                            {
                                if (pEntity->getAllowFlight())
                                {
                                    pEntity->setAllowFlight(false);
                                }
                            }
                        }
                        config_fly_on_tty = false;
                    } else
                    {
                        config_fly_on_tty = json_msg["allow_fly_on_territory"];
                    }
                    if (json_msg["money_connect"]) {
                        if (json_msg["price"] > 0) {
                            config_money_connect = true;
                            config_price = json_msg["price"];
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("经济配置错误,检查价格是否大于0;领地价格依然保持默认", localeP));
                        }
                    }
                } else {
                    sender.sendErrorMessage(LangTty.getLocal("配置文件错误,使用默认配置", localeP));
                }
            } catch (const std::exception& e) {
                sender.sendErrorMessage(LangTty.getLocal("配置文件错误,使用默认配置", localeP)+","+e.what());
            }
            sender.sendMessage(LangTty.getLocal("重载领地配置和数据完成", localeP));
        } else if (args[0] == "set") {
            try {
                if (!args[1].empty() && !args[2].empty() && !args[3].empty()) {
                    const string& tty_name = args[3];
                    if (TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                        sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                    } else {
                        int per_val;
                        if (args[2] == "true") {
                            per_val = 1;
                        } else {
                            per_val = 0;
                        }
                        if (auto [fst, snd] = action_->change_territory_permissions(tty_name, args[1], per_val); fst) {
                            sender.sendMessage(snd);
                        } else {
                            sender.sendErrorMessage(snd);
                        }
                    }
                } else {
                    sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
                }
            } catch (const std::exception &e) {
                sender.sendErrorMessage(e.what());
            }
        } else if (args[0] == "transfer") {
            if (!args[1].empty() && !args[2].empty()) {
                if (const auto tty_data = Territory_Action::read_territory_by_name(args[1]); tty_data != nullptr) {
                    if (auto [fst, snd] = action_->change_territory_owner(args[1], tty_data->owner, args[2]); fst) {
                        sender.sendMessage(snd);
                    } else {
                        sender.sendErrorMessage(snd);
                    }
                } else {
                    sender.sendErrorMessage(LangTty.getLocal("未知的领地", localeP));
                }
            } else {
                sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
            }
        } else if (args[0] == "transfer_all") {
            if (!args[1].empty() && !args[2].empty()) {
                if (auto tty_info = Territory_Action::list_player_tty(args[1]); tty_info.empty()) {
                    sender.sendErrorMessage(LangTty.getLocal("该玩家没有领地", localeP));
                } else {
                    sender.sendMessage(
                            LangTty.getLocal("正在转让玩家 ", localeP) + args[1] + LangTty.getLocal(" 的领地给 ", localeP) + args[2]);
                    for (const auto &the_tty: tty_info) {
                        if (auto [fst, snd] = action_->change_territory_owner(the_tty.name, args[1], args[2]); fst) {
                            sender.sendMessage(LangTty.getLocal("成功转让领地: ", localeP) + the_tty.name);
                        } else {
                            sender.sendErrorMessage(LangTty.getLocal("转让领地失败: ", localeP) + the_tty.name);
                        }
                    }
                }
            } else {
                sender.sendErrorMessage(LangTty.getLocal("缺少参数", localeP));
            }
        }
    }
    return true;
}