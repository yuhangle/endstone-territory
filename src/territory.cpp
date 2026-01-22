//
// Created by yuhang on 2025/3/6.
//

#include "territory.h"
#include "translate.hpp"
#include "version.h"

//初始化其它实例
DataBase Database(db_file);
Territory_Action TA(Database);
translate LangTty;

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
void Territory::datafile_check() const {
    json df_config = {
            {"player_max_tty_num", 20},
            {"actor_fire_attack_protect", true},
            {"money_with_umoney", false},
            {"price", 1},
            {"max_tty_area",4000000},
            {"welcome_all",true},
            {"language","zh_CN"}
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
                    getLogger().info(LangTty.tr(LangTty.getLocal("配置项 '{}' 已根据默认配置进行更新"),key));
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

// 从数据库读取所有领地数据
void Territory::readAllTerritories() {
    (void)TA.get_all_tty();
}

// 提示领地信息函数
void Territory::tips_online_players() const {
    for (auto online_player_list = getServer().getOnlinePlayers(); const auto &player : online_player_list) {
        string player_name = player->getName();
        string player_dim = player->getLocation().getDimension()->getName();
        Territory_Action::Point3D player_pos = {player->getLocation().getBlockX(), player->getLocation().getBlockY(),
                              player->getLocation().getBlockZ()};

        // 检查玩家位置是否有变化
        if (!welcome_all && get<0>(lastPlayerPositions[player_name]) == player_pos) {
            // 玩家位置未改变
            continue;
        }
        if (get<0>(lastPlayerPositions[player_name]) == tuple{0.000000, 0.000000, 0.000000}) {
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
                if (auto pEntity = getServer().getPlayer(player_name); pEntity != nullptr) {
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
                if (auto pEntity = getServer().getPlayer(player_name); pEntity != nullptr) {
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
bool Territory::del_player_tty(const std::string &tty_name) const {
    const auto tty_data = Territory_Action::read_territory_by_name(tty_name);
    const int area = Territory_Action::get_tty_area(static_cast<int>(get<0>(tty_data->pos1)),static_cast<int>(get<2>(tty_data->pos1)),static_cast<int>(get<0>(tty_data->pos2)),static_cast<int>(get<2>(tty_data->pos2)));
    string owner;
    if (money_with_umoney) {
        (void)umoney_change_player_money(tty_data->owner,area*price);
        if (const auto the_player = getServer().getPlayer(tty_data->owner)) {
            owner = the_player->getName();
            the_player->sendMessage(LangTty.getLocal("您的领地已被删除,以当前价格返还资金:") + to_string(area*price));
        }
    }
    if (TA.del_Tty_by_name(tty_name)) {
        return true;
    }
    if (!owner.empty())
    {
        (void)umoney_change_player_money(owner,area*price);
    }
    return false;
}

//检查插件存在
bool Territory::umoney_check_exists() const {
    if (getServer().getPluginManager().getPlugin("umoney")) {
        return true;
    }
    return false;
}

//获取玩家资金
int Territory::umoney_get_player_money(const std::string& player_name) {
    std::ifstream f(umoney_file);
    if (!f.is_open()) {
        std::cerr << "Error: Could not open file: " << umoney_file << std::endl;
        return -1; // 返回一个错误值表示文件无法打开
    }

    try {
        if (json data = json::parse(f); data.contains(player_name)) {
            return data[player_name].get<int>();
        }
        std::cerr << "Warning: Player '" << player_name << "' not found in the data." << std::endl;
        return 0; // 返回 0 表示玩家不存在或没有资金记录
    } catch (const json::parse_error& e) {
        std::cerr << "Error: JSON parse error in file '" << umoney_file << "': " << e.what() << std::endl;
        return -1; // 返回一个错误值表示 JSON 解析失败
    }

    return -1;
}

//更改玩家资金
bool Territory::umoney_change_player_money(const std::string& player_name, const int money) const {
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
void Territory::onLoad()
{
    getLogger().info("onLoad is called");
    //执行数据目录检查
    datafile_check();
    (void)Database.init_database();
    if (!std::filesystem::exists(language_path+"lang.json"))
    {
        LangTty = translate(language_path+getServer().getLanguage().getLocale()+".json");
        LangTty.loadLanguage();
    }
}

void Territory::onEnable()
{
    getLogger().info("onEnable is called");

    json json_msg = read_config();
    try {
        if (!json_msg.contains("error")) {
            max_tty_num = json_msg["player_max_tty_num"];
            actor_fire_attack_protect = json_msg["actor_fire_attack_protect"];
            max_tty_area = json_msg["max_tty_area"];
            welcome_all = json_msg["welcome_all"];
            language = json_msg["language"];
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
            welcome_all = true;
        }
    } catch (const std::exception& e) {
        max_tty_num = 20;
        actor_fire_attack_protect = true;
        money_with_umoney = false;
        price = 1;
        max_tty_area = 4000000;
        welcome_all = true;
        getLogger().error(LangTty.getLocal("配置文件错误,使用默认配置")+","+e.what());
    }
    LangTty = translate(language_path+language+".json");LangTty.loadLanguage();
    translate::checkLanguageCommon(language_path+language+".json",language_file);
    //注册事件监听
    registerEvent<endstone::BlockBreakEvent>(onBlockBreak);
    registerEvent<endstone::BlockPlaceEvent>(onBlockPlace);
    registerEvent<endstone::ActorExplodeEvent>(onActorBomb);
    registerEvent<endstone::PlayerInteractEvent>(onPlayerjiaohu);
    registerEvent<endstone::PlayerInteractActorEvent>(onPlayerjiaohust);
    registerEvent<endstone::ActorDamageEvent>(onActorhit);
    //快速创建领地选择监听
    registerEvent<endstone::PlayerInteractEvent>(quickCreateTtyRightClick);
    //数据库读取
    readAllTerritories();
    //周期执行
    getServer().getScheduler().runTaskTimer(*this,[&]() { tips_online_players(); }, 0, 25);
    menu_ = std::make_unique<Menu>(*this);

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
    const auto [fst, snd] = LangTty.loadLanguage();
    getLogger().info(snd);
    const auto enable_msg = endstone::ColorFormat::Yellow + LangTty.getLocal("Territory插件已启用, Version: ") + getServer().getPluginManager().getPlugin("territory")->getDescription().getVersion();
    getLogger().info(enable_msg);
    getLogger().info(endstone::ColorFormat::Yellow + "Project address: https://github.com/yuhangle/endstone-territory");
}

void Territory::onDisable()
{
    getLogger().info("onDisable is called");
    getServer().getScheduler().cancelTasks(*this);
}

bool Territory::onCommand(endstone::CommandSender &sender, const endstone::Command &command, const std::vector<std::string> &args) {
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
                            sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地"));
                            return false;
                        }
                        //维度
                        string dim = getServer().getPlayer(player_name)->getLocation().getDimension()->getName();
                        
                        // 检查领地大小
                        const int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
                        if (area >= max_tty_area && max_tty_area != -1) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建"));
                            return false;
                        }
                        
                        // 检查玩家领地数量是否达到上限
                        if (Territory_Action::check_tty_num(player_name) >= max_tty_num) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地"));
                            return false;
                        }
                        // 检查资金
                        if (money_with_umoney) {
                            const int money = umoney_get_player_money(player_name);
                            if (const int value = area * price; money >= value) {
                                (void)umoney_change_player_money(player_name,-value);
                                sender.sendMessage(LangTty.getLocal("设置领地已扣费:") + to_string(value));
                                // 调用领地创建函数
                                if (auto [success, message] = TA.create_territory(player_name, pos1, pos2, tppos, dim); success) {
                                    sender.sendMessage(LangTty.getLocal("成功添加领地"));
                                    readAllTerritories();
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal(message));
                                    (void)umoney_change_player_money(player_name,value); // 创建失败返还资金
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("你的资产不足以设置此大小的领地,设置此领地所需要的资金为:") +
                                                                             to_string(value));
                                return false;
                            }
                        }
                        // 不使用经济
                        else {
                            // 调用领地创建函数
                            if (auto [success, message] = TA.create_territory(player_name, pos1, pos2, tppos, dim); success) {
                                sender.sendMessage(LangTty.getLocal("成功添加领地"));
                                readAllTerritories();
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal(message));
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
                            sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地"));
                            return false;
                        }
                        //维度
                        string dim = getServer().getPlayer(player_name)->getLocation().getDimension()->getName();
                        
                        // 检查领地大小
                        const int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
                        if (area >= max_tty_area && max_tty_area != -1) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建"));
                            return false;
                        }
                        
                        // 检查玩家领地数量是否达到上限
                        if (Territory_Action::check_tty_num(player_name) >= max_tty_num) {
                            sender.sendErrorMessage(LangTty.getLocal("你的领地数量已达到上限,无法增加新的领地"));
                            return false;
                        }

                        // 检查资金
                        if (money_with_umoney) {
                            const int money = umoney_get_player_money(player_name);
                            if (const int value = area * price; money >= value) {
                                (void)umoney_change_player_money(player_name,-value);
                                sender.sendMessage(LangTty.getLocal("设置领地已扣费:") + to_string(value));
                                // 调用子领地创建函数
                                if (auto [success, child_territory_name] = TA.create_sub_territory(player_name, pos1, pos2, tppos, dim); success) {
                                    sender.sendMessage(LangTty.tr("成功添加子领地,归属于父领地: ", child_territory_name));
                                    readAllTerritories();
                                } else {
                                    sender.sendErrorMessage(LangTty.getLocal(child_territory_name)); // child_territory_name here contains error message
                                    (void)umoney_change_player_money(player_name,value); // 创建失败返还资金
                                }
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal("你的资产不足以设置此大小的领地,设置此领地所需要的资金为:") +
                                                                             to_string(value));
                                return false;
                            }
                        }
                        else {
                            // 调用子领地创建函数
                            if (auto [success, child_territory_name] = TA.create_sub_territory(player_name, pos1, pos2, tppos, dim); success) {
                                sender.sendMessage(LangTty.tr("成功添加子领地,归属于父领地: ", child_territory_name));
                                readAllTerritories();
                            } else {
                                sender.sendErrorMessage(LangTty.getLocal(child_territory_name)); // child_territory_name here contains error message
                            }
                        }
                    } catch (const std::exception &e) {
                        sender.sendErrorMessage(e.what());
                    }
                } else if (args[0] == "list") {
                    try {
                        string player_name = sender.getName();
                        if (auto territories = Territory_Action::list_player_tty(player_name); territories.empty()) {
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
                            if (const auto tty_data = Territory_Action::read_territory_by_name(args[1]); tty_data != nullptr) {
                                if (Territory_Action::check_tty_owner(tty_data->name, player_name) == true) {
                                    if (del_player_tty(tty_data->name)) {
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
                        if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                            sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                        } else {
                            if (!args[1].empty() && !args[2].empty()) {
                                if (Territory_Action::check_tty_owner(args[1], player_name) == true) {
                                    if (auto [fst, snd] = TA.rename_player_tty(args[1], args[2]); fst) {
                                        sender.sendMessage(snd);
                                    } else {
                                        sender.sendErrorMessage(snd);
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
                            if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                if (Territory_Action::check_tty_op(tty_name, player_name) == true) {
                                    int per_val;
                                    if (args[2] == "true") {
                                        per_val = 1;
                                    } else {
                                        per_val = 0;
                                    }
                                    if (auto [fst, snd] = TA.change_territory_permissions(tty_name, args[1], per_val); fst) {
                                        sender.sendMessage(snd);
                                    } else {
                                        sender.sendErrorMessage(snd);
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
                            if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                if (Territory_Action::check_tty_op(tty_name, player_name) == true) {
                                    if (args[1] == "add") {
                                        if (auto [fst, snd] = TA.change_territory_member(tty_name, "add", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
                                        }
                                    } else if (args[1] == "remove") {
                                        if (auto [fst, snd] = TA.change_territory_member(tty_name, "remove", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
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
                            if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                if (Territory_Action::check_tty_owner(tty_name, player_name) == true) {
                                    if (args[1] == "add") {
                                        if (auto [fst, snd] = TA.change_territory_manager(tty_name, "add", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
                                        }
                                    } else if (args[1] == "remove") {
                                        if (auto [fst, snd] = TA.change_territory_manager(tty_name, "remove", args[2]); fst) {
                                            sender.sendMessage(snd);
                                        } else {
                                            sender.sendErrorMessage(snd);
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
                                if (auto [fst, snd] = TA.change_tty_tppos(tty_name, tp_pos, dim); fst) {
                                    sender.sendMessage(snd);
                                } else {
                                    sender.sendErrorMessage(snd);
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
                            if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                            } else {
                                if (Territory_Action::check_tty_owner(tty_name, player_name) == true) {
                                    if (auto [fst, snd] = TA.change_territory_owner(tty_name, player_name, args[2]); fst) {
                                        sender.sendMessage(snd);
                                    } else {
                                        sender.sendErrorMessage(snd);
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
                            if (Territory_Action::TerritoryData *tty_info = Territory_Action::read_territory_by_name(tty_name); tty_info != nullptr) {
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
                } else if (args[0] == "resize") {
                    try
                    {
                        if (!args[1].empty() && !args[2].empty() && !args[3].empty())
                        {
                            auto tty_data = Territory_Action::read_territory_by_name(args[1]);
                            if (tty_data == nullptr)
                            {
                                sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                                return false;
                            }
                            if (tty_data->owner != sender.getName())
                            {
                                sender.sendErrorMessage(LangTty.getLocal("你没有权限变更此领地"));
                                return false;
                            }
                            //领地位置1
                            Territory_Action::Point3D pos1 = Territory_Action::pos_to_tuple(args[2]);
                            //领地位置2
                            Territory_Action::Point3D pos2 = Territory_Action::pos_to_tuple(args[3]);
                            //传送坐标
                            Territory_Action::Point3D tppos = Territory_Action::pos_to_tuple(to_string(sender.asPlayer()->getLocation().getX()) + " " + to_string(sender.asPlayer()->getLocation().getY()) + " " + to_string(sender.asPlayer()->getLocation().getZ()));
                            if (get<1>(pos1) > 320 || get<1>(pos1) < -64 || get<1>(pos2) > 320 || get<1>(pos2) < -64) {
                                sender.sendErrorMessage(LangTty.getLocal("无法在世界之外设置领地"));
                                return false;
                            }

                            // 检查领地大小
                            const int area = Territory_Action::get_tty_area(static_cast<int>(std::get<0>(pos1)),static_cast<int>(std::get<2>(pos1)),static_cast<int>(std::get<0>(pos2)),static_cast<int>(std::get<2>(pos2)));
                            if (area >= max_tty_area && max_tty_area != -1) {
                                sender.sendErrorMessage(LangTty.getLocal("你的领地大小超过所能创建的最大面积,无法创建"));
                                return false;
                            }
                            // 资金检查
                            int changed_money = 0;
                            if (money_with_umoney)
                            {
                                const int old_area = Territory_Action::get_tty_area(static_cast<int>(get<0>(tty_data->pos1)),static_cast<int>(get<2>(tty_data->pos1)),static_cast<int>(get<0>(tty_data->pos2)),static_cast<int>(get<2>(tty_data->pos2)));
                                const int old_value = old_area * price;
                                const int new_value = area * price;
                                changed_money = new_value - old_value;
                                if (umoney_get_player_money(sender.getName()) < changed_money)
                                {
                                    sender.sendErrorMessage(LangTty.getLocal("你没有足够的资金"));
                                    return false;
                                }
                                if (changed_money != 0)
                                {
                                    (void)umoney_change_player_money(sender.getName(), -changed_money);
                                }
                            }

                            if (auto [fst, snd] = TA.resize_territory(pos1,pos2,*tty_data,tppos); fst)
                            {
                                sender.sendMessage(LangTty.getLocal(snd));
                                readAllTerritories();
                            }  else
                            {
                                sender.sendErrorMessage(LangTty.getLocal(snd));
                                if (changed_money!=0)
                                {
                                    (void)umoney_change_player_money(sender.getName(), changed_money);
                                }
                            }

                        } else
                        {
                            sender.sendErrorMessage(LangTty.getLocal("缺少参数"));
                        }
                    } catch (const std::exception &e)
                    {
                        sender.sendErrorMessage(e.what());
                    }
                }
                else if (args[0] == "help") {
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
                    if (auto tty_info = Territory_Action::list_player_tty(args[1]); tty_info.empty()) {
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
                        if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                            getLogger().error(LangTty.getLocal("未知的领地"));
                        } else {
                            int per_val;
                            if (args[2] == "true") {
                                per_val = 1;
                            } else {
                                per_val = 0;
                            }
                            auto [fst, snd] = TA.change_territory_permissions(tty_name, args[1], per_val);
                            if (fst) {
                                getLogger().info(snd);
                            } else {
                                getLogger().error(snd);
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
                    if (auto tty_info = Territory_Action::list_player_tty(args[1]); tty_info.empty()) {
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
                        if (Territory_Action::TerritoryData *tty_data = Territory_Action::read_territory_by_name(tty_name); tty_data == nullptr) {
                            sender.sendErrorMessage(LangTty.getLocal("未知的领地"));
                        } else {
                            int per_val;
                            if (args[2] == "true") {
                                per_val = 1;
                            } else {
                                per_val = 0;
                            }
                            if (auto [fst, snd] = TA.change_territory_permissions(tty_name, args[1], per_val); fst) {
                                sender.sendMessage(snd);
                            } else {
                                sender.sendErrorMessage(snd);
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
void Territory::onBlockBreak(endstone::BlockBreakEvent& event)
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension()->getName();
    const Territory_Action::Point3D block_pos = {event.getBlock().getLocation().getBlockX(),event.getBlock().getLocation().getBlockY(),event.getBlock().getLocation().getBlockZ()};
    //检查玩家是否在领地上
    if (const auto player_in_tty = Territory_Action::list_in_tty(block_pos,player_dim); player_in_tty != std::nullopt) {
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
void Territory::onBlockPlace(endstone::BlockPlaceEvent& event)
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension()->getName();
    const Territory_Action::Point3D block_pos = {event.getBlock().getLocation().getBlockX(),event.getBlock().getLocation().getBlockY(),event.getBlock().getLocation().getBlockZ()};
    //检查玩家是否在领地上
    if (const auto player_in_tty = Territory_Action::list_in_tty(block_pos,player_dim); player_in_tty != std::nullopt) {
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
void Territory::onPlayerjiaohu(endstone::PlayerInteractEvent& event)
{
    if (!event.getBlock()) {
        return;
    }
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension()->getName();
    const Territory_Action::Point3D block_pos = {event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
    //检查玩家是否在领地上
    if (const auto player_in_tty = Territory_Action::list_in_tty(block_pos,player_dim); player_in_tty != std::nullopt) {
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
void Territory::onPlayerjiaohust(endstone::PlayerInteractActorEvent& event)
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension()->getName();
    const Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
    //检查玩家是否在领地上
    if (const auto player_in_tty = Territory_Action::list_in_tty(actor_pos,player_dim); player_in_tty != std::nullopt) {
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
void Territory::onActorBomb(endstone::ActorExplodeEvent& event)
{
    const string actor_dim = event.getActor().getLocation().getDimension()->getName();

    //先检查实体是否在领地上
    const Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
    if (const auto actor_in_tty = Territory_Action::list_in_tty(actor_pos,actor_dim); actor_in_tty != std::nullopt) {
        for (const auto&info : actor_in_tty.value()) {
            if (!(info.if_bomb)) {
                event.setCancelled(true);
                return;
            }
        }
    }

    //再检查方块是否在领地上
    auto& broken_blocks = event.getBlockList();  // vector<unique_ptr<endstone::Block>>&

    std::erase_if(
        broken_blocks,
        [&](const std::unique_ptr<endstone::Block>& block) {
            const Territory_Action::Point3D block_pos = {
                block->getX(),
                block->getY(),
                block->getZ()
            };

            if (const auto block_in_tty = Territory_Action::list_in_tty(block_pos, actor_dim);
                block_in_tty != std::nullopt) {

                // 检查是否有任何领地将此方块标记为不允许爆炸
                for (const auto& info : block_in_tty.value()) {
                    if (!info.if_bomb) {
                        return true;
                    }
                }
            }
            return false;
        }
    );
}

//实体受击
void Territory::onActorhit(endstone::ActorDamageEvent& event)
{
    const string actor_dim = event.getActor().getLocation().getDimension()->getName();
    const Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
    //检查实体是否在领地上
    if (const auto actor_in_tty = Territory_Action::list_in_tty(actor_pos,actor_dim); actor_in_tty != std::nullopt) {
        for (const auto&info : actor_in_tty.value()) {
            if (!(info.if_damage)) {
                // 玩家导致伤害
                if (const auto actor_or_player = event.getDamageSource().getActor(); actor_or_player && actor_or_player->getType() == "minecraft:player") {
                    if (ranges::find(info.members,actor_or_player->getName()) == info.members.end()) {
                        actor_or_player->sendErrorMessage(LangTty.getLocal("你没有在此领地上伤害的权限"));
                        event.setCancelled(true);
                    }
                    else
                    {
                        entity_can_die.push_back(event.getActor().getId());
                    }
                    //火焰攻击类伤害
                } else if (event.getDamageSource().getType() == "fire_tick") {
                    //非玩家实体才免疫
                    if (event.getActor().getType() != "minecraft:player") {
                        if (actor_fire_attack_protect) {
                            if (ranges::find(entity_can_die, event.getActor().getId()) == entity_can_die.end())
                            {
                                event.setCancelled(true);
                            }
                        }
                    }
                }
            }
            //外部爆炸伤害
            if (!(info.if_bomb))
            {
                if (event.getDamageSource().getType() == "entity_explosion")
                {
                    event.setCancelled(true);
                }
            }
            return;
        }
    }
}

//实体死亡
void Territory::onActorDeath(const endstone::ActorDeathEvent& event)
{
    if (ranges::find(entity_can_die, event.getActor().getId()) != entity_can_die.end())
    {
        entity_can_die.erase(ranges::find(entity_can_die, event.getActor().getId()));
    }
}

//快速创建领地-右键事件
void Territory::quickCreateTtyRightClick(const endstone::PlayerInteractEvent& event) {
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
                    "/tty (set)<opt: opt_setper> (if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage)<opt: opt_permission> <bool: bool> <territory: str>",
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
                    "/optty (set)<opt: opt_op_setper> (if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage)<opt: opt_op_permission> <bool: bool> <msg: message>",
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