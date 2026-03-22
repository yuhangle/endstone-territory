//
// Created by yuhang on 2026/3/20.
//

#include "event_listener.h"
#include "territory_action.h"
#include "territory.h"

using namespace std;

EventListener::EventListener(Territory* territory, translate& lang_tty)
    : plugin_(*territory),
      lang_tty_(lang_tty),
      territory_(territory)
{}

// 事件监听
//方块破坏监听
void EventListener::onBlockBreak(endstone::BlockBreakEvent& event) const
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D block_pos = {
        event.getBlock().getLocation().getBlockX(),
        event.getBlock().getLocation().getBlockY(),
        event.getBlock().getLocation().getBlockZ()
    };
    if (const auto result = TerritoryInstance::canBreak(player_name, block_pos, player_dim); result.has_value() && !result.value()) {
        event.setCancelled(true);
        event.getPlayer().sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上破坏的权限"));
    }
}

//方块放置监听
void EventListener::onBlockPlace(endstone::BlockPlaceEvent& event) const
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D block_pos = {
        event.getBlock().getLocation().getBlockX(),
        event.getBlock().getLocation().getBlockY(),
        event.getBlock().getLocation().getBlockZ()
    };
    if (const auto result = TerritoryInstance::canBuild(player_name, block_pos, player_dim); result.has_value() && !result.value()) {
        event.setCancelled(true);
        event.getPlayer().sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上放置的权限"));
    }
}

//玩家交互监听
void EventListener::onPlayerjiaohu(endstone::PlayerInteractEvent& event) const
{
    if (!event.getBlock()) return;
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D block_pos = {
        event.getBlock()->getLocation().getBlockX(),
        event.getBlock()->getLocation().getBlockY(),
        event.getBlock()->getLocation().getBlockZ()
    };
    if (const auto result = TerritoryInstance::canInteract(player_name, block_pos, player_dim); result.has_value() && !result.value()) {
        event.setCancelled(true);
        event.getPlayer().sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上交互的权限"));
    }
}

//玩家实体交互监听
void EventListener::onPlayerjiaohust(endstone::PlayerInteractActorEvent& event) const
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D actor_pos = {
        event.getActor().getLocation().getBlockX(),
        event.getActor().getLocation().getBlockY(),
        event.getActor().getLocation().getBlockZ()
    };
    if (const auto result = TerritoryInstance::canInteract(player_name, actor_pos, player_dim); result.has_value() && !result.value()) {
        event.setCancelled(true);
        event.getPlayer().sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上交互的权限"));
    }
}

//实体爆炸监听
void EventListener::onActorBomb(endstone::ActorExplodeEvent& event) {
    const string actor_dim = event.getActor().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D actor_pos = {
        event.getActor().getLocation().getBlockX(),
        event.getActor().getLocation().getBlockY(),
        event.getActor().getLocation().getBlockZ()
    };
    // 检查实体位置是否允许爆炸
    if (const auto actorResult = TerritoryInstance::canExplode(actor_pos, actor_dim); actorResult.has_value() && !actorResult.value()) {
        event.setCancelled(true);
        return;
    }

    // 过滤被破坏的方块
    auto& broken_blocks = event.getBlockList();
    std::erase_if(broken_blocks, [&](const std::unique_ptr<endstone::Block>& block) {
        const TerritoryInstance::Point3D block_pos = {block->getX(), block->getY(), block->getZ()};
        const auto blockResult = TerritoryInstance::canExplode(block_pos, actor_dim);
        return blockResult.has_value() && !blockResult.value();
    });
}

// 领地边缘活塞监听
void EventListener::onEdgePiston(endstone::BlockPistonEvent& event) {
    const string piston_dim = event.getBlock().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D piston_pos = {
        static_cast<double>(event.getBlock().getLocation().getBlockX()),
        static_cast<double>(event.getBlock().getLocation().getBlockY()),
        static_cast<double>(event.getBlock().getLocation().getBlockZ())
    };
    if (const auto result = TerritoryInstance::canPiston(piston_pos, piston_dim); result.has_value() && !result.value()) {
        event.setCancelled(true);
    }
}

//实体受击
void EventListener::onActorhit(endstone::ActorDamageEvent& event) const
{
    const string actor_dim = event.getActor().getLocation().getDimension().getName();
    const Territory_Action::Point3D actor_pos = {event.getActor().getLocation().getBlockX(),event.getActor().getLocation().getBlockY(),event.getActor().getLocation().getBlockZ()};
    //检查实体是否在领地上
    if (const auto actor_in_tty = Territory_Action::list_in_tty(actor_pos,actor_dim); actor_in_tty != std::nullopt) {
        for (const auto&info : actor_in_tty.value()) {
            if (!(info.if_damage)) {
                // 玩家导致伤害
                if (const auto actor_or_player = event.getDamageSource().getActor(); actor_or_player && actor_or_player->getType() == "minecraft:player") {
                    if (ranges::find(info.members,actor_or_player->getName()) == info.members.end()) {
                        if (const auto player = actor_or_player->asPlayer())
                        {
                            player->sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上伤害的权限"));
                        }
                        event.setCancelled(true);
                    }
                    else
                    {
                        territory_->config_entity_can_die.push_back(event.getActor().getId());
                    }
                    //火焰攻击类伤害
                } else if (event.getDamageSource().getType() == "fire_tick") {
                    //非玩家实体才免疫
                    if (event.getActor().getType() != "minecraft:player") {
                        if (territory_->config_actor_fire_attack_protect) {
                            if (ranges::find(territory_->config_entity_can_die, event.getActor().getId()) == territory_->config_entity_can_die.end())
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
void EventListener::onActorDeath(const endstone::ActorDeathEvent& event) const
{
    if (ranges::find(territory_->config_entity_can_die, event.getActor().getId()) != territory_->config_entity_can_die.end())
    {
        territory_->config_entity_can_die.erase(ranges::find(territory_->config_entity_can_die, event.getActor().getId()));
    }
}

//快速创建领地-右键事件
void EventListener::quickCreateTtyRightClick(const endstone::PlayerInteractEvent& event) const
{
    if (!territory_->quick_create_player_data.contains(event.getPlayer().getName())) {
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
        auto tmp = territory_->quick_create_player_data[player.getName()];
        if (tmp.dim1.empty()) {
            tmp.dim1 = player.getDimension().getName();
            tmp.pos1 = Territory_Action::Point3D{event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
            player.sendMessage(lang_tty_.getLocal("已记录此坐标为第一个坐标，请选择第二个坐标"));
        } else if (!tmp.dim2.empty()){
            //点二已存在数据，去重
            return;
        } else {
            tmp.dim2 = player.getDimension().getName();
            //进行维度检查
            if (tmp.dim1 != tmp.dim2) {
                player.sendErrorMessage(lang_tty_.getLocal("坐标在不同维度，无法创建领地"));
                return;
            }
            tmp.pos2 = Territory_Action::Point3D{event.getBlock()->getLocation().getBlockX(),event.getBlock()->getLocation().getBlockY(),event.getBlock()->getLocation().getBlockZ()};
            //由于交互事件可能多次触发，检查点重复
            if (tmp.pos1 == tmp.pos2) {
                return;
            }
            player.sendMessage(lang_tty_.getLocal("已记录此坐标为第二个坐标，请通过提示完成领地创建"));
        }
        //存储坐标数据
        territory_->quick_create_player_data[player.getName()] = tmp;
        if (!tmp.dim2.empty()) {
            territory_->menu_->openQuickCreateTtyMenu(&player,tmp);
        }
    }
}

void EventListener::onPlayerMove(endstone::PlayerMoveEvent& event)
{
    auto& player = event.getPlayer();
    std::string player_name = player.getName();

    //时间节流逻辑
    auto now = std::chrono::steady_clock::now();

    // 检查是否已经记录过该玩家且间隔小于 1 秒
    if (lastProcessTime.contains(player_name)) {
        if (auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProcessTime[player_name]); duration < CHECK_INTERVAL) {
            return;
        }
    }

    // 更新该玩家的最后处理时间
    lastProcessTime[player_name] = now;

    // 获取移动后的位置信息
    auto to_loc = event.getTo();
    std::string player_dim = to_loc.getDimension().getName();

    // 构造当前位置的 Point3D (tuple<double, double, double>)
    Territory_Action::Point3D player_pos = {
        static_cast<double>(to_loc.getBlockX()),
        static_cast<double>(to_loc.getBlockY()),
        static_cast<double>(to_loc.getBlockZ())
    };

    // 获取该玩家上一次记录的状态引用
    auto& [last_pos, last_tty, last_father] = territory_->lastPlayerPositions[player_name];

    // 1. 检查坐标变化
    if (!territory_->config_welcome_all && last_pos == player_pos) {
        return;
    }

    // 2. 初始化检查 (对应 tuple{0.0, 0.0, 0.0})
    if (std::get<0>(last_pos) == 0.0 && std::get<1>(last_pos) == 0.0 && std::get<2>(last_pos) == 0.0) {
        last_pos = player_pos;
        return;
    }

    // 更新位置记录
    std::string previous_territory = last_tty;
    std::string previous_father_territory = last_father;
    last_pos = player_pos;

    // 3. 寻找当前最精细的领地
    const auto& all_tty = Territory_Action::getAllTty();
    const TerritoryData* selectedTerritory = nullptr;

    for (const auto &val : all_tty | std::views::values) {
        const auto &data = val;
        if (data.dim == player_dim && Territory_Action::isPointInCube(player_pos, data.pos1, data.pos2)) {
            if (selectedTerritory == nullptr) {
                selectedTerritory = &data;
            } else {
                // 子领地优先逻辑
                if (!data.father_tty.empty() && selectedTerritory->father_tty.empty()) {
                    selectedTerritory = &data;
                }
            }
        }
    }

    // 4. 发送提示与权限处理
    if (selectedTerritory != nullptr) {
        std::string current_territory = selectedTerritory->name;
        std::string current_father_territory = selectedTerritory->father_tty;

        // 领地名改变或强制欢迎
        if (previous_territory != current_territory || territory_->config_welcome_all) {
            std::string msg;
            bool is_sub = !current_father_territory.empty();

            std::string action_str = territory_->config_welcome_all ?
                lang_tty_.getLocal("§2[领地] §r您当前位于 ") :
                lang_tty_.getLocal("§2[领地] §r欢迎来到 ");

            std::string type_str = is_sub ?
                lang_tty_.getLocal(" 的子领地 ") :
                lang_tty_.getLocal(" 的领地 ");

            msg = action_str + selectedTerritory->owner + type_str + current_territory;
            player.sendTip(msg);

            // 飞行校验
            if (territory_->config_fly_on_tty) {
                if (Territory_Action::is_tty_op(current_territory, player_name).value()) {
                    player.setAllowFlight(true);
                } else {
                    if (int gm = static_cast<int>(player.getGameMode()); (gm == 0 || gm == 3) && player.getAllowFlight()) {
                        player.setAllowFlight(false);
                    }
                }
            }

            // 更新记录值
            last_tty = current_territory;
            last_father = current_father_territory;
        }
    }
    else {
        // 5. 离开领地逻辑
        if (!previous_territory.empty()) {
            std::string msg = lang_tty_.getLocal("§2[领地] §r您已离开领地 ") + previous_territory + lang_tty_.getLocal(", 欢迎下次再来");
            player.sendTip(msg);

            if (territory_->config_fly_on_tty) {
                if (int gm = static_cast<int>(player.getGameMode()); (gm == 0 || gm == 3) && player.getAllowFlight()) {
                    player.setAllowFlight(false);
                }
            }

            // 清空记录值
            last_tty = "";
            last_father = "";
        }
    }
}

void EventListener::onEnititySummon(endstone::ActorSpawnEvent& event) const
{
    if (const auto& actor = event.getActor(); ranges::any_of(territory_->no_allow_entitys,
                                                       [&](const std::string& s) { return s == actor.getType(); }))
    {
        const string actor_dim = actor.getLocation().getDimension().getName();
        const TerritoryInstance::Point3D actor_pos = {
            static_cast<double>(actor.getLocation().getBlockX()),
            static_cast<double>(actor.getLocation().getBlockY()),
            static_cast<double>(actor.getLocation().getBlockZ())
        };
        if (const auto result = TerritoryInstance::canWitherExist(actor_pos, actor_dim); result.has_value() && !result.value()) {
            event.setCancelled(true);
        }
    }
}
