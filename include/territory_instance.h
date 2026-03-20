//
// Created by yuhang on 2026/3/20.
//

#ifndef TERRITORY_TERRITORY_INSTANCE_H
#define TERRITORY_TERRITORY_INSTANCE_H

#include <string>
#include <tuple>
#include <vector>
#include <optional>
#include "territory_types.h"

class Territory_Action;

class TerritoryInstance {
public:
    using Point3D = std::tuple<double, double, double>;

    // 构造函数
    TerritoryInstance(std::string name, Territory_Action* action);

    // --- 获取器 ---
    [[nodiscard]] std::string getName() const { return name_; }
    [[nodiscard]] std::optional<Point3D> getPos1() const;
    [[nodiscard]] std::optional<Point3D> getPos2() const;
    [[nodiscard]] std::optional<Point3D> getTpPos() const;
    [[nodiscard]] std::optional<std::string> getOwner() const;
    [[nodiscard]] std::optional<std::string> getDim() const;
    [[nodiscard]] std::optional<std::string> getFather() const;
    [[nodiscard]] std::optional<std::vector<std::string>> getMembers() const;
    [[nodiscard]] std::optional<std::vector<std::string>> getManagers() const;
    [[nodiscard]] std::optional<bool> hasPermission(const std::string& node) const;
    [[nodiscard]] bool isValid() const;

    // --- 修改器 (返回 pair<bool, string> 提供详细结果) ---
    // 权限设置
    [[nodiscard]] std::pair<bool, std::string> setPermission(const std::string& node, bool value) const;

    // 成员管理
    [[nodiscard]] std::pair<bool, std::string> addMember(const std::string& player) const;
    [[nodiscard]] std::pair<bool, std::string> removeMember(const std::string& player) const;

    // 管理员管理
    [[nodiscard]] std::pair<bool, std::string> addManager(const std::string& player) const;
    [[nodiscard]] std::pair<bool, std::string> removeManager(const std::string& player) const;

    // 所有权转移
    [[nodiscard]] std::pair<bool, std::string> transferOwner(const std::string& newOwner) const;

    // 重命名
    std::pair<bool, std::string> rename(const std::string& newName);

    // 传送点设置
    [[nodiscard]] std::pair<bool, std::string> setTpPos(const Point3D& pos, const std::string& dim) const;

    // 调整领地大小
    [[nodiscard]] std::pair<bool, std::string> resize(const Point3D& newPos1, const Point3D& newPos2, const Point3D& newTpPos) const;

    // 删除领地
    [[nodiscard]] bool remove() const;

    // --- 静态权限检查方法 ---
    // 检查玩家能否破坏指定位置的方块
    [[nodiscard]] static std::optional<bool> canBreak(const std::string& player, const Point3D& pos, const std::string& dim);
    // 检查玩家能否放置方块
    [[nodiscard]] static std::optional<bool> canBuild(const std::string& player, const Point3D& pos, const std::string& dim);
    // 检查玩家能否与方块或实体交互
    [[nodiscard]] static std::optional<bool> canInteract(const std::string& player, const Point3D& pos, const std::string& dim);
    // 检查指定位置是否允许爆炸（用于爆炸事件及爆炸伤害）
    [[nodiscard]] static std::optional<bool> canExplode(const Point3D& pos, const std::string& dim);
    // 检查玩家能否对实体造成伤害
    [[nodiscard]] static std::optional<bool> canPlayerDamage(const std::string& damager, const Point3D& pos, const std::string& dim);
    // 检查活塞是否允许在领地边缘工作
    [[nodiscard]] static std::optional<bool> canPiston(const Point3D& pos, const std::string& dim);


private:
    std::string name_;
    Territory_Action* action_;  // 不拥有，仅用于调用操作函数

    // 辅助函数：获取当前领地数据，如果不存在返回 nullptr
    [[nodiscard]] TerritoryData* getData() const;
};

#endif //TERRITORY_TERRITORY_INSTANCE_H