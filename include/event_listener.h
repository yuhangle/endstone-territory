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
    void onPlayerMove(endstone::PlayerMoveEvent& event);
private:
    endstone::Plugin &plugin_;
    // 记录每个玩家上次触发领地逻辑的时间
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> lastProcessTime;
    // 检查间隔：1秒
    const std::chrono::milliseconds CHECK_INTERVAL{1000};
};


#endif //TERRITORY_EVENT_LISTENER_H