//
// Created by yuhang on 2026/3/20.
//

#ifndef TERRITORY_EVENT_LISTENER_H
#define TERRITORY_EVENT_LISTENER_H
#include <endstone/endstone.hpp>

class EventListener
{
public:
    explicit EventListener(endstone::Plugin& plugin) : plugin_(plugin) {}

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

    //领地边缘活塞监听
    static void onEdgePiston(endstone::BlockPistonEvent& event);

    //快速创建领地-右键事件
    static void quickCreateTtyRightClick(const endstone::PlayerInteractEvent& event);

    //玩家移动事件
    static void onPlayerMove(endstone::PlayerMoveEvent& event);
private:
    endstone::Plugin &plugin_;
};


#endif //TERRITORY_EVENT_LISTENER_H