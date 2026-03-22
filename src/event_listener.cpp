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
{
    global_checker_ = std::make_unique<TerritoryInstance>("GLOBAL_CHECKER", territory->action_.get());
}

// 事件监听
//方块破坏监听
void EventListener::onBlockBreak(endstone::BlockBreakEvent& event) const
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension().getName();

    // 构造坐标
    const TerritoryInstance::Point3D block_pos = {
        static_cast<double>(event.getBlock().getLocation().getBlockX()),
        static_cast<double>(event.getBlock().getLocation().getBlockY()),
        static_cast<double>(event.getBlock().getLocation().getBlockZ())
    };
    if (const auto result = global_checker_->canBreak(player_name, block_pos, player_dim);
        result.has_value() && !result.value())
    {
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
        static_cast<double>(event.getBlock().getLocation().getBlockX()),
        static_cast<double>(event.getBlock().getLocation().getBlockY()),
        static_cast<double>(event.getBlock().getLocation().getBlockZ())
    };

    if (const auto result = global_checker_->canBuild(player_name, block_pos, player_dim);
        result.has_value() && !result.value())
    {
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
        static_cast<double>(event.getBlock()->getLocation().getBlockX()),
        static_cast<double>(event.getBlock()->getLocation().getBlockY()),
        static_cast<double>(event.getBlock()->getLocation().getBlockZ())
    };

    if (const auto result = global_checker_->canInteract(player_name, block_pos, player_dim); result.has_value() && !result.value()) {
        event.setCancelled(true);
        event.getPlayer().sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上交互的权限"));
    }
}

//玩家实体交互监听
void EventListener::onPlayerjiaohust(endstone::PlayerInteractActorEvent& event) const
{
    const string player_name = event.getPlayer().getName();
    const string player_dim = event.getPlayer().getLocation().getDimension().getName();

    // 获取实体当前所在位置
    const TerritoryInstance::Point3D actor_pos = {
        static_cast<double>(event.getActor().getLocation().getBlockX()),
        static_cast<double>(event.getActor().getLocation().getBlockY()),
        static_cast<double>(event.getActor().getLocation().getBlockZ())
    };

    if (const auto result = global_checker_->canInteract(player_name, actor_pos, player_dim);
        result.has_value() && !result.value())
    {
        event.setCancelled(true);
        event.getPlayer().sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上交互的权限"));
    }
}

//实体爆炸监听
void EventListener::onActorBomb(endstone::ActorExplodeEvent& event) const
{
    const string actor_dim = event.getActor().getLocation().getDimension().getName();
    const TerritoryInstance::Point3D actor_pos = {
        static_cast<double>(event.getActor().getLocation().getBlockX()),
        static_cast<double>(event.getActor().getLocation().getBlockY()),
        static_cast<double>(event.getActor().getLocation().getBlockZ())
    };

    if (const auto actorResult = global_checker_->canExplode(actor_pos, actor_dim);
        actorResult.has_value() && !actorResult.value())
    {
        event.setCancelled(true);
        return;
    }

    auto& broken_blocks = event.getBlockList();
    std::erase_if(broken_blocks, [&](const std::unique_ptr<endstone::Block>& block) {
        const TerritoryInstance::Point3D block_pos = {
            static_cast<double>(block->getX()),
            static_cast<double>(block->getY()),
            static_cast<double>(block->getZ())
        };

        const auto blockResult = global_checker_->canExplode(block_pos, actor_dim);
        return blockResult.has_value() && !blockResult.value();
    });
}

// 领地边缘活塞监听
void EventListener::onEdgePiston(endstone::BlockPistonEvent& event) const
{
    const string piston_dim = event.getBlock().getLocation().getDimension().getName();

    // 构造活塞位置坐标
    const TerritoryInstance::Point3D piston_pos = {
        static_cast<double>(event.getBlock().getLocation().getBlockX()),
        static_cast<double>(event.getBlock().getLocation().getBlockY()),
        static_cast<double>(event.getBlock().getLocation().getBlockZ())
    };
    if (const auto result = global_checker_->canPiston(piston_pos, piston_dim);
        result.has_value() && !result.value())
    {
        event.setCancelled(true);
    }
}

//实体受击
void EventListener::onActorhit(endstone::ActorDamageEvent& event) const
{
    const string actor_dim = event.getActor().getLocation().getDimension().getName();

    // 构造受击实体坐标
    const TerritoryInstance::Point3D actor_pos = {
        static_cast<double>(event.getActor().getLocation().getBlockX()),
        static_cast<double>(event.getActor().getLocation().getBlockY()),
        static_cast<double>(event.getActor().getLocation().getBlockZ())
    };
    if (const auto actor_in_tty = territory_->action_->list_in_tty(actor_pos, actor_dim); actor_in_tty.has_value()) {

        for (const auto& info : actor_in_tty.value()) {
            if (!info.if_damage) {
                if (const auto damager_actor = event.getDamageSource().getActor()) {
                    if (damager_actor->getType() == "minecraft:player") {
                        if (!info.source->cached_all_members.contains(damager_actor->getName())) {
                            if (const auto* player = damager_actor->asPlayer()) {
                                player->sendPopup(endstone::ColorFormat::Red + lang_tty_.getLocal("你没有在此领地上伤害的权限"));
                            }
                            event.setCancelled(true);
                        } else {
                            // 允许伤害，记录 ID
                            territory_->config_entity_can_die.push_back(event.getActor().getId());
                        }
                    }
                }
                // B. 火焰攻击类伤害（环境伤害）
                else if (event.getDamageSource().getType() == "fire_tick") {
                    if (event.getActor().getType() != "minecraft:player") {
                        if (territory_->config_actor_fire_attack_protect) {
                            // 检查该实体是否已被允许死亡（比如被主人点燃）
                            if (std::ranges::find(territory_->config_entity_can_die, event.getActor().getId()) == territory_->config_entity_can_die.end()) {
                                event.setCancelled(true);
                            }
                        }
                    }
                }
            }

            // 2. 检查爆炸权限
            if (!info.if_bomb) {
                if (event.getDamageSource().getType() == "entity_explosion") {
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

    // 时间节流逻辑
    auto now = std::chrono::steady_clock::now();
    if (lastProcessTime.contains(player_name)) {
        if (auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastProcessTime[player_name]); duration < CHECK_INTERVAL) {
            return;
        }
    }
    lastProcessTime[player_name] = now;

    auto to_loc = event.getTo();
    std::string player_dim = to_loc.getDimension().getName();
    TerritoryInstance::Point3D player_pos = {
        static_cast<double>(to_loc.getBlockX()),
        static_cast<double>(to_loc.getBlockY()),
        static_cast<double>(to_loc.getBlockZ())
    };

    auto& [last_pos, last_tty, last_father] = territory_->lastPlayerPositions[player_name];

    if (!territory_->config_welcome_all && last_pos == player_pos) return;
    last_pos = player_pos;

    // 利用空间索引寻找当前领地
    const auto res = territory_->action_->list_in_tty(player_pos, player_dim);

    const TerritoryData* selectedTerritory = nullptr;
    if (res.has_value() && !res->empty()) {
        selectedTerritory = (*res)[0].source;
    }

    if (selectedTerritory != nullptr) {
        std::string current_territory = selectedTerritory->name;
        std::string current_father_territory = selectedTerritory->father_tty;

        if (last_tty != current_territory || territory_->config_welcome_all) {
            bool is_sub = !current_father_territory.empty();

            std::string action_str = territory_->config_welcome_all ?
                lang_tty_.getLocal("§2[领地] §r您当前位于 ") :
                lang_tty_.getLocal("§2[领地] §r欢迎来到 ");

            std::string type_str = is_sub ?
                lang_tty_.getLocal(" 的子领地 ") :
                lang_tty_.getLocal(" 的领地 ");

            player.sendTip(action_str + selectedTerritory->owner + type_str + current_territory);

            // 飞行校验
            if (territory_->config_fly_on_tty) {
                // 利用我们之前做的 cached_operators 优化性能
                if (selectedTerritory->cached_operators.contains(player_name)) {
                    player.setAllowFlight(true);
                } else {
                    if (int gm = static_cast<int>(player.getGameMode()); (gm == 0 || gm == 3) && player.getAllowFlight()) {
                        player.setAllowFlight(false);
                    }
                }
            }

            last_tty = current_territory;
            last_father = current_father_territory;
        }
    }
    else {
        // 离开领地逻辑
        if (!last_tty.empty()) {
            player.sendTip(lang_tty_.getLocal("§2[领地] §r您已离开领地 ") + last_tty + lang_tty_.getLocal(", 欢迎下次再来"));

            if (territory_->config_fly_on_tty) {
                if (int gm = static_cast<int>(player.getGameMode()); (gm == 0 || gm == 3) && player.getAllowFlight()) {
                    player.setAllowFlight(false);
                }
            }
            last_tty = "";
            last_father = "";
        }
    }
}

void EventListener::onEnititySummon(endstone::ActorSpawnEvent& event) const
{
    if (const auto& actor = event.getActor(); std::ranges::any_of(territory_->no_allow_entitys,
                                                                  [&](const std::string& s) { return s == actor.getType(); }))
    {
        const string actor_dim = actor.getLocation().getDimension().getName();
        const TerritoryInstance::Point3D actor_pos = {
            static_cast<double>(actor.getLocation().getBlockX()),
            static_cast<double>(actor.getLocation().getBlockY()),
            static_cast<double>(actor.getLocation().getBlockZ())
        };

        if (const auto result = global_checker_->canWitherExist(actor_pos, actor_dim);
            result.has_value() && !result.value())
        {
            event.setCancelled(true);
        }
    }
}
