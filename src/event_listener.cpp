//
// Created by yuhang on 2026/3/20.
//

#include "event_listener.h"
#include "territory_action.h"
#include "territory.h"
#include <binarystream/BinaryStream.hpp>

using namespace bstream;
using namespace std;

EventListener::EventListener(Territory* territory, translate& lang_tty)
    : plugin_(*territory),
      lang_tty_(lang_tty),
      territory_(territory)
{
    global_checker_ = std::make_unique<TerritoryInstance>("GLOBAL_CHECKER", territory->action_.get());
}

/**
 * 基础粒子发包函数
 */
void spawnParticlePacket(const endstone::Player& player, const float x, const float y, const float z, const int event_id, const int data = 0) {
    BinaryStream stream;

    // 1. Event ID (VarInt)
    stream.writeVarInt(event_id);
    // 2. Position (3个 Float)
    stream.writeFloat(x);
    stream.writeFloat(y);
    stream.writeFloat(z);
    // 3. Data (VarInt)
    stream.writeVarInt(data);

    player.sendPacket(25, stream.getAndReleaseData());
}

//渲染领地边界墙
void renderTerritoryGroundLine(const endstone::Player& player,
                               const TerritoryData& territory,
                               const endstone::Location& loc) {
    const auto [p1x, p1y, p1z] = territory.pos1;
    const auto [p2x, p2y, p2z] = territory.pos2;
    const double min_x = std::min(p1x, p2x);
    const double max_x = std::max(p1x, p2x);
    const double min_z = std::min(p1z, p2z);
    const double max_z = std::max(p1z, p2z);

    const double px = loc.getX();
    const double py = loc.getY();
    const double pz = loc.getZ();

    constexpr int effect_id = 16403; // 亮橙火焰
    constexpr double perimeter_limit = 100.0;
    const double width = max_x - min_x;
    const double length = max_z - min_z;
    const bool render_full = (2.0 * (width + length)) < perimeter_limit;

    auto spawn_quad_layered = [&](const double x, const double z) {
        const auto fx = static_cast<float>(x + 0.5);
        const auto fz = static_cast<float>(z + 0.5);
        const auto base_y = static_cast<float>(py);

        // 统一 4 层高度分布
        spawnParticlePacket(player, fx, base_y - 1.2f, fz, effect_id, 0); // 足部
        spawnParticlePacket(player, fx, base_y + 0.8f, fz, effect_id, 0); // 胸部
        spawnParticlePacket(player, fx, base_y + 4.8f, fz, effect_id, 0); // 高空
        spawnParticlePacket(player, fx, base_y + 9.8f, fz, effect_id, 0); // 顶端
    };

    if (render_full) {
        const int count_x = static_cast<int>(std::floor(width));
        const int count_z = static_cast<int>(std::floor(length));

        for (int i = 0; i <= count_x; ++i) {
            const double cur_x = min_x + static_cast<double>(i);
            spawn_quad_layered(cur_x, min_z);
            spawn_quad_layered(cur_x, max_z);
        }
        for (int i = 0; i <= count_z; ++i) {
            const double cur_z = min_z + static_cast<double>(i);
            spawn_quad_layered(min_x, cur_z);
            spawn_quad_layered(max_x, cur_z);
        }
    } else {
        constexpr int partial_range = 50;
        constexpr int step = 4;

        const double d_west = std::abs(px - min_x);
        const double d_east = std::abs(px - max_x);
        const double d_north = std::abs(pz - min_z);
        const double d_south = std::abs(pz - max_z);

        const double min_d = std::min({d_west, d_east, d_north, d_south});
        if (min_d > 6.0) return;

        if (min_d == d_west || min_d == d_east) {
            const double target_x = (min_d == d_west) ? min_x : max_x;
            const double center_z = std::floor(pz);
            for (int dz = -partial_range; dz <= partial_range; dz += step) {
                if (const double cur_z = center_z + static_cast<double>(dz);
                    cur_z >= min_z && cur_z <= max_z) {
                    spawn_quad_layered(target_x, cur_z);
                }
            }
        } else {
            const double target_z = (min_d == d_north) ? min_z : max_z;
            const double center_x = std::floor(px);
            for (int dx = -partial_range; dx <= partial_range; dx += step) {
                if (const double cur_x = center_x + static_cast<double>(dx);
                    cur_x >= min_x && cur_x <= max_x) {
                    spawn_quad_layered(cur_x, target_z);
                }
            }
        }
    }
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

        // 领地变更
        if (bool territory_changed = (last_tty != current_territory); territory_changed || territory_->config_welcome_all) {
            // 渲染粒子墙
            if (territory_changed) {
                renderTerritoryGroundLine(player, *selectedTerritory, to_loc);
            }

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
        // 离开领地
        if (!last_tty.empty()) {
            // 获取离开的领地数据
            if (TerritoryData* leaving_territory = Territory_Action::read_territory_by_name(last_tty)) {
                renderTerritoryGroundLine(player, *leaving_territory, to_loc);
            }

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
