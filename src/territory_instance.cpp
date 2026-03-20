//
// Created by yuhang on 2026/3/20.
//

#include "territory_instance.h"
#include "territory_action.h"
#include "database.hpp"
#include <algorithm>

TerritoryInstance::TerritoryInstance(std::string name, Territory_Action* action)
    : name_(std::move(name)), action_(action)
{}

TerritoryData* TerritoryInstance::getData() const {
    return Territory_Action::read_territory_by_name(name_);
}

bool TerritoryInstance::isValid() const {
    return getData() != nullptr;
}

// --- Getters ---
std::optional<TerritoryInstance::Point3D> TerritoryInstance::getPos1() const {
    if (auto* data = getData()) return data->pos1;
    return std::nullopt;
}

std::optional<TerritoryInstance::Point3D> TerritoryInstance::getPos2() const {
    if (auto* data = getData()) return data->pos2;
    return std::nullopt;
}

std::optional<TerritoryInstance::Point3D> TerritoryInstance::getTpPos() const {
    if (auto* data = getData()) return data->tppos;
    return std::nullopt;
}

std::optional<std::string> TerritoryInstance::getOwner() const {
    if (auto* data = getData()) return data->owner;
    return std::nullopt;
}

std::optional<std::string> TerritoryInstance::getDim() const {
    if (auto* data = getData()) return data->dim;
    return std::nullopt;
}

std::optional<std::string> TerritoryInstance::getFather() const {
    if (auto* data = getData()) return data->father_tty;
    return std::nullopt;
}

std::optional<std::vector<std::string>> TerritoryInstance::getMembers() const {
    if (const auto* data = getData()) {
        if (data->member.empty()) return std::vector<std::string>();
        return DataBase::splitString(data->member);
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> TerritoryInstance::getManagers() const {
    if (const auto* data = getData()) {
        if (data->manager.empty()) return std::vector<std::string>();
        return DataBase::splitString(data->manager);
    }
    return std::nullopt;
}

std::optional<bool> TerritoryInstance::hasPermission(const std::string& node) const {
    if (auto* data = getData()) {
        if (node == "if_jiaohu") return data->if_jiaohu;
        if (node == "if_break") return data->if_break;
        if (node == "if_build") return data->if_build;
        if (node == "if_tp") return data->if_tp;
        if (node == "if_bomb") return data->if_bomb;
        if (node == "if_damage") return data->if_damage;
        if (node == "if_edge_piston") return data->if_edge_piston;
    }
    return std::nullopt;
}

// --- Modifiers ---
std::pair<bool, std::string> TerritoryInstance::setPermission(const std::string& node, bool value) const
{
    // 直接调用 action 的权限修改函数
    return action_->change_territory_permissions(name_, node, value ? 1 : 0);
}

std::pair<bool, std::string> TerritoryInstance::addMember(const std::string& player) const
{
    return action_->change_territory_member(name_, "add", player);
}

std::pair<bool, std::string> TerritoryInstance::removeMember(const std::string& player) const
{
    return action_->change_territory_member(name_, "remove", player);
}

std::pair<bool, std::string> TerritoryInstance::addManager(const std::string& player) const
{
    return action_->change_territory_manager(name_, "add", player);
}

std::pair<bool, std::string> TerritoryInstance::removeManager(const std::string& player) const
{
    return action_->change_territory_manager(name_, "remove", player);
}

std::pair<bool, std::string> TerritoryInstance::transferOwner(const std::string& newOwner) const
{
    // 需要获取当前主人名称
    const auto ownerOpt = getOwner();
    if (!ownerOpt.has_value()) {
        return {false, "领地不存在"};
    }
    return action_->change_territory_owner(name_, *ownerOpt, newOwner);
}

std::pair<bool, std::string> TerritoryInstance::rename(const std::string& newName) {
    auto result = action_->rename_player_tty(name_, newName);
    if (result.first) {
        name_ = newName;
    }
    return result;
}

std::pair<bool, std::string> TerritoryInstance::setTpPos(const Point3D& pos, const std::string& dim) const
{
    return action_->change_tty_tppos(name_, pos, dim);
}

std::pair<bool, std::string> TerritoryInstance::resize(const Point3D& newPos1, const Point3D& newPos2, const Point3D& newTpPos) const
{
    const auto* data = getData();
    if (!data) {
        return {false, "领地不存在"};
    }
    return action_->resize_territory(newPos1, newPos2, *data, newTpPos);
}

bool TerritoryInstance::remove() const
{
    // 调用删除玩家领地的函数（带经济返还等）
    return action_->del_player_tty(name_);
}

// --- 静态权限检查方法 ---
std::optional<bool> TerritoryInstance::canBreak(const std::string& player, const Point3D& pos, const std::string& dim) {
    const auto ttyList = Territory_Action::list_in_tty(pos, dim);
    if (!ttyList.has_value()) {
        return std::nullopt;
    }
    // 遍历所有匹配的领地，只要有一个禁止且玩家不是成员，则禁止
    for (const auto& info : ttyList.value()) {
        if (!info.if_break) {
            if (std::ranges::find(info.members, player) == info.members.end()) {
                return false;  // 权限关闭且玩家不在成员中 → 禁止
            }
        }
    }
    return true;  // 所有领地都允许或玩家在成员中
}

std::optional<bool> TerritoryInstance::canBuild(const std::string& player, const Point3D& pos, const std::string& dim) {
    const auto ttyList = Territory_Action::list_in_tty(pos, dim);
    if (!ttyList.has_value()) {
        return std::nullopt;
    }
    for (const auto& info : ttyList.value()) {
        if (!info.if_build) {
            if (std::ranges::find(info.members, player) == info.members.end()) {
                return false;
            }
        }
    }
    return true;
}

std::optional<bool> TerritoryInstance::canInteract(const std::string& player, const Point3D& pos, const std::string& dim) {
    const auto ttyList = Territory_Action::list_in_tty(pos, dim);
    if (!ttyList.has_value()) {
        return std::nullopt;
    }
    for (const auto& info : ttyList.value()) {
        if (!info.if_jiaohu) {
            if (std::ranges::find(info.members, player) == info.members.end()) {
                return false;
            }
        }
    }
    return true;
}

std::optional<bool> TerritoryInstance::canExplode(const Point3D& pos, const std::string& dim) {
    const auto ttyList = Territory_Action::list_in_tty(pos, dim);
    if (!ttyList.has_value()) {
        return std::nullopt;
    }
    // 爆炸权限：只要有一个领地禁止爆炸，整个点就禁止
    for (const auto& info : ttyList.value()) {
        if (!info.if_bomb) {
            return false;
        }
    }
    return true;
}

std::optional<bool> TerritoryInstance::canPlayerDamage(const std::string& damager, const Point3D& pos, const std::string& dim) {
    const auto ttyList = Territory_Action::list_in_tty(pos, dim);
    if (!ttyList.has_value()) {
        return std::nullopt;
    }
    for (const auto& info : ttyList.value()) {
        if (!info.if_damage) {
            if (std::ranges::find(info.members, damager) == info.members.end()) {
                return false;
            }
        }
    }
    return true;
}

std::optional<bool> TerritoryInstance::canPiston(const Point3D& pos, const std::string& dim) {
    const auto ttyList = Territory_Action::list_in_tty(pos, dim);
    if (!ttyList.has_value()) {
        return std::nullopt;
    }
    // 活塞边缘权限：需要检查点是否在领地的边缘
    for (const auto& info : ttyList.value()) {
        if (!info.if_edge_piston) {
            // 如果权限关闭，检查点是否在边缘
            if (Territory_Action::check_in_tty_edge(info.name, pos)) {
                return false;
            }
        }
    }
    return true;
}
