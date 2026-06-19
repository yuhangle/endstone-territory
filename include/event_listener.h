//
// Created by yuhang on 2026/3/20.
//

#ifndef TERRITORY_EVENT_LISTENER_H
#define TERRITORY_EVENT_LISTENER_H
#include <endstone/endstone.hpp>
#include "translate.hpp"
#include <territory_instance.h>

class Territory;

class EventListener
{
public:
    explicit EventListener(Territory* territory, translate& lang_tty);
    friend class Territory;

    // 事件监听
    //方块破坏监听
    void onBlockBreak(endstone::BlockBreakEvent& event) const;

    //方块放置监听
    void onBlockPlace(endstone::BlockPlaceEvent& event) const;

    //玩家交互监听
    void onPlayerjiaohu(endstone::PlayerInteractEvent& event) const;

    //玩家实体交互监听
    void onPlayerjiaohust(endstone::PlayerInteractActorEvent& event) const;

    //实体爆炸监听
    void onActorBomb(endstone::ActorExplodeEvent& event) const;

    //实体受击
    void onActorhit(endstone::ActorDamageEvent& event) const;

    //实体死亡
    void onActorDeath(const endstone::ActorDeathEvent& event) const;

    //领地边缘活塞监听
    void onEdgePiston(endstone::BlockPistonEvent& event) const;

    //快速创建领地-右键事件
    void quickCreateTtyRightClick(const endstone::PlayerInteractEvent& event) const;

    //玩家移动事件
    void onPlayerMove(endstone::PlayerMoveEvent& event);

    //实体生成事件
    void onEnititySummon(endstone::ActorSpawnEvent& event) const;
private:
    endstone::Plugin &plugin_;
    translate& lang_tty_;
    Territory* territory_;
    // 记录每个玩家上次触发领地逻辑的时间
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastProcessTime;
    // 检查间隔：1秒
    const std::chrono::milliseconds CHECK_INTERVAL{1000};
    std::unique_ptr<TerritoryInstance> global_checker_;
    //检查op是否有权限
    bool check_op_p(const endstone::Player& player) const;
};


#endif //TERRITORY_EVENT_LISTENER_H