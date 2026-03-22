// territory_types.h
#ifndef TERRITORY_TYPES_H
#define TERRITORY_TYPES_H
#include <string>
#include <tuple>
#include <database.hpp>
#include <unordered_set>

struct TerritoryData {
    std::string name;
    std::tuple<double, double, double> pos1 = {0,0,0};
    std::tuple<double, double, double> pos2 = {0,0,0};
    std::tuple<double, double, double> tppos = {0,0,0};
    std::string owner;
    std::string manager;
    std::string member;
    std::string dim;
    std::string father_tty;

    // --- 权限标识 ---
    bool if_jiaohu = false;
    bool if_break = false;
    bool if_tp = false;
    bool if_build = false;
    bool if_bomb = false;
    bool if_damage = false;
    bool if_edge_piston = false;
    bool if_wither = false;

    // --- 新增：性能缓存字段 ---
    // 包含 owner + manager + member
    std::unordered_set<std::string> cached_all_members;
    // 仅包含 owner + manager (用于以后的管理权限校验)
    std::unordered_set<std::string> cached_operators;

    /**
     * @brief 在从数据库加载完原始字符串后调用此方法
     * 建议在插件启动加载数据或领地信息变更（如添加成员）时调用一次
     */
    void updateCache() {
        cached_all_members.clear();
        cached_operators.clear();

        const auto owners = DataBase::splitString(owner);
        const auto managers = DataBase::splitString(manager);
        const auto members = DataBase::splitString(member);

        // 填充管理员缓存 (Owner + Manager)
        for (const auto& player : owners) {
            if (!player.empty()) {
                cached_operators.insert(player);
                cached_all_members.insert(player);
            }
        }
        for (const auto& player : managers) {
            if (!player.empty()) {
                cached_operators.insert(player);
                cached_all_members.insert(player);
            }
        }

        // 填充普通成员 (仅加入全员集合)
        for (const auto& player : members) {
            if (!player.empty()) {
                cached_all_members.insert(player);
            }
        }
    }
};

struct InTtyInfo {
    std::string name;
    const TerritoryData* source;

    std::string owner;
    bool if_jiaohu;
    bool if_break;
    bool if_build;
    bool if_bomb;
    bool if_damage;
    bool if_edge_piston;
    bool if_wither;
};

struct GridKey {
    int x, z;
    bool operator==(const GridKey& other) const {
        return x == other.x && z == other.z;
    }
};

// 为 GridKey 提供哈希函数，以便存入 unordered_map
struct GridKeyHash {
    std::size_t operator()(const GridKey& k) const {
        return std::hash<int>()(k.x) ^ (std::hash<int>()(k.z) << 1);
    }
};

struct ScopedTimer {
    std::string name;
    std::chrono::high_resolution_clock::time_point start;

    // 构造时记录开始时间
    explicit ScopedTimer(std::string timer_name)
        : name(std::move(timer_name)),
          start(std::chrono::high_resolution_clock::now()) {}

    // 析构时自动计算耗时并打印
    ~ScopedTimer() {
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        const std::chrono::duration<double, std::micro> duration_us = duration_ns;

        std::cout << std::format("[Performance] {} took {} ns ({:.3f} us)\n",
                                 name, duration_ns.count(), duration_us.count());
    }
};
#endif