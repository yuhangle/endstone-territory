//
// Created by yuhang on 2025/9/7.
//

#ifndef TERRITORY_MENU_H
#define TERRITORY_MENU_H
#pragma once
#include <endstone/endstone.hpp>
#include "territory_action.h"

class Menu {
public:
  explicit Menu(endstone::Plugin &plugin) : plugin_(plugin) {}

  [[nodiscard]] vector<std::string> getOnlinePlayerList() const;
  
  // 管理领地菜单
  void openManTtyMenu(endstone::Player* player) const;

  // 主菜单按钮补充
  void openMainMenu(endstone::Player* player) const;

  void openCreateTtyMenu(endstone::Player* player) const;

  void openCreateSubTtyMenu(endstone::Player* player) const;

  //快速创建领地主菜单
  void openQuickCreateMainMenu(endstone::Player*  player) const;

  // 重命名领地菜单
  void openRenameTtyMenu(endstone::Player* player, const std::vector<std::string>& ttyNames) const;

  // 传送自己及已加入的领地菜单
  void openTpTtyMenu(endstone::Player* player) const;

  // 传送全部领地菜单
  void openTpAllTtyMenu(endstone::Player* player) const;

  // 列出自己的全部领地
  static void openListTtyMenu(endstone::Player* player);

  // 管理自己管理的领地权限
  void openSetPermisMenu(endstone::Player* player) const;

  // 领地权限详细设置
  void openSetPermisDetailMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const;

  // 删除自己管理的领地成员
  void openDelTtyMemberMenu(endstone::Player* player) const;

  // 删除成员子菜单
  void openDelTtyMemberSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const;

  // 添加自己管理的领地成员
  void openAddTtyMemberMenu(endstone::Player* player) const;

  // 添加成员子菜单
  void openAddTtyMemberSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const;

  // 删除自己领地的领地管理员
  void openDelTtyManagerMenu(endstone::Player* player) const;

  // 删除管理员子菜单
  void openDelTtyManagerSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const;

  // 添加自己领地的领地管理员
  void openAddTtyManagerMenu(endstone::Player* player) const;

  // 添加管理员子菜单
  void openAddTtyManagerSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const;

  // 设置自己管理的领地的传送点
  void openSetTpTtyMenu(endstone::Player* player) const;

  // 转让领地菜单
  void openTransferTtyMenu(endstone::Player* player) const;

  // 删除领地菜单
  void openDelTtyMenu(endstone::Player* player) const;

  //快速创建领地菜单
  static void openQuickCreateTtyMenu(endstone::Player* player,Territory_Action::QuickTtyData& quick_tty_data);

private:
  endstone::Plugin &plugin_;
};
#endif //TERRITORY_MENU_H