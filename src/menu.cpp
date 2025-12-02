//
// Created by yuhang on 2025/12/2.
//
#include "menu.h"
#include "territory.h"

vector<std::string> Menu::getOnlinePlayerList() const {
  const auto players = plugin_.getServer().getOnlinePlayers();
  vector<string> onlinePlayerNames;
  onlinePlayerNames.reserve(players.size());
  for (const auto& player : players) {
    onlinePlayerNames.push_back(player->getName());
  }
  return onlinePlayerNames;
}

// 管理领地菜单
void Menu::openManTtyMenu(endstone::Player* player) const {
  endstone::ActionForm menu;
  menu.setTitle(LangTty.getLocal("§l领地管理界面"));

  // 管理权限
  endstone::Button setPermisBtn(
    LangTty.getLocal("§l§1管理自己管理的领地权限"),
    "textures/ui/accessibility_glyph_color",
    [this](endstone::Player* p) { openSetPermisMenu(p); }
  );
  // 删除成员
  endstone::Button delMemberBtn(
    LangTty.getLocal("§l§1删除自己管理的领地成员"),
    "textures/ui/permissions_member_star",
    [this](endstone::Player* p) { openDelTtyMemberMenu(p); }
  );
  // 添加成员
  endstone::Button addMemberBtn(
    LangTty.getLocal("§l§1添加自己管理的领地成员"),
    "textures/ui/permissions_member_star_hover",
    [this](endstone::Player* p) { openAddTtyMemberMenu(p); }
  );

  // 删除管理员
  endstone::Button delManagerBtn(
    LangTty.getLocal("§l§1删除自己领地的领地管理员"),
    "textures/ui/permissions_op_crown",
    [this](endstone::Player* p) { openDelTtyManagerMenu(p); }
  );
  // 添加管理员
  endstone::Button addManagerBtn(
    LangTty.getLocal("§l§1添加自己领地的领地管理员"),
    "textures/ui/permissions_op_crown_hover",
    [this](endstone::Player* p) { openAddTtyManagerMenu(p); }
  );
  // 设置传送点
  endstone::Button setTpBtn(
    LangTty.getLocal("§l§1设置自己管理的领地的传送点"),
    "textures/ui/csb_purchase_warning",
    [this](endstone::Player* p) { openSetTpTtyMenu(p); }
  );
  // 转让领地
  endstone::Button transferBtn(
    LangTty.getLocal("§l§1将自己的领地转让给其他玩家"),
    "textures/ui/trade_icon",
    [this](endstone::Player* p) { openTransferTtyMenu(p); }
  );
  // 删除领地
  endstone::Button delBtn(
    LangTty.getLocal("§l§4删除自己的领地"),
    "textures/ui/book_trash_default",
    [this](endstone::Player* p) { openDelTtyMenu(p); }
  );
  menu.setControls({setPermisBtn, setTpBtn, addMemberBtn, delMemberBtn, addManagerBtn, delManagerBtn, transferBtn, delBtn});

  menu.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });

  player->sendForm(menu);
}

// 主菜单按钮补充
void Menu::openMainMenu(endstone::Player* player) const{
  endstone::ActionForm menu;
  menu.setTitle(LangTty.getLocal("§5Territory领地菜单"));
  menu.addButton(LangTty.getLocal("§l§5新建领地"), "textures/ui/color_plus", [this] (endstone::Player* p) {
    openCreateTtyMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5创建子领地"), "textures/ui/copy", [this] (endstone::Player* p) {
    openCreateSubTtyMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5快速创建"), "textures/ui/welcome", [this] (endstone::Player* p) {
    openQuickCreateMainMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5重命名领地"), "textures/ui/book_edit_default", [this] (endstone::Player* p) {
    auto names = Territory_Action::getPlayerTtyNames(p->getName());
    if (names.empty()) {
      p->sendErrorMessage(LangTty.getLocal("未查找到领地"));
      return;
    }
    openRenameTtyMenu(p, names);
  });
  menu.addButton(LangTty.getLocal("§l§5传送自己及已加入的领地"), "textures/ui/csb_purchase_warning", [this] (endstone::Player* p) {
    openTpTtyMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5传送全部领地"), "textures/ui/default_world", [this] (endstone::Player* p) {
    openTpAllTtyMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5管理领地"), "textures/ui/icon_setting", [this] (endstone::Player* p) {
    openManTtyMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5列出自己的全部领地"), "textures/ui/infobulb", [] (endstone::Player* p) {
    openListTtyMenu(p);
  });
  menu.addButton(LangTty.getLocal("§l§5查看领地帮助"), "textures/ui/Feedback", [] (const endstone::Player* p) {
    (void)p->performCommand("tty help");
  });

  if (player) {
    player->sendForm(menu);
  }
}

void Menu::openCreateTtyMenu(endstone::Player* player) const {
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l§5新建领地"));

  // 输入框1
  endstone::TextInput inputTtyPos1;
  inputTtyPos1.setLabel(LangTty.getLocal("§l输入领地边角坐标1 格式示例: 114 5 14"));

  // 输入框2
  endstone::TextInput inputTtyPos2;
  inputTtyPos2.setLabel(LangTty.getLocal("§l输入领地边角坐标2 格式示例: 191 98 10"));

  // 标签
  endstone::Label infoLabel;
  infoLabel.setText(LangTty.getLocal("三维领地为立方体,需要领地的两个对角坐标,并注意高度需要覆盖领地"));

  // 添加控件
  form.addControl(inputTtyPos1);
  form.addControl(inputTtyPos2);
  form.addControl(infoLabel);

  // 提交回调
  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      if (parsed[0].empty() || parsed[1].empty()) {
        p->sendErrorMessage(LangTty.getLocal("输入不能为空"));
        return;
      }
      const Territory_Action::Point3D pos1 = Territory_Action::pos_to_tuple(parsed[0]);
      const Territory_Action::Point3D pos2 = Territory_Action::pos_to_tuple(parsed[1]);
      std::ostringstream cmd;
      cmd << "tty add " << get<0>(pos1) << " " << get<1>(pos1) << " " << get<2>(pos1) << " " << get<0>(pos2) << " " << get<1>(pos2) << " " << get<2>(pos2);
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("错误的坐标!"));
    }
  });

  // 关闭回调
  form.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });

  player->sendForm(form);
}

void Menu::openCreateSubTtyMenu(endstone::Player* player) const {
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l§5新建子领地"));

  endstone::TextInput inputSubTtyPos1;
  inputSubTtyPos1.setLabel(LangTty.getLocal("§l输入子领地边角坐标1 格式示例: 114 5 14"));

  endstone::TextInput inputSubTtyPos2;
  inputSubTtyPos2.setLabel(LangTty.getLocal("§l输入子领地边角坐标2 格式示例: 191 98 10"));

  endstone::Label infoLabel;
  infoLabel.setText(LangTty.getLocal("子领地需要在父领地之内创建,不能超出父领地,只有父领地的所有者和管理员有权限创建"));

  form.addControl(inputSubTtyPos1);
  form.addControl(inputSubTtyPos2);
  form.addControl(infoLabel);

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      if (parsed[0].empty() || parsed[1].empty()) {
        p->sendErrorMessage(LangTty.getLocal("输入不能为空"));
        return;
      }
      const Territory_Action::Point3D pos1 = Territory_Action::pos_to_tuple(parsed[0]);
      const Territory_Action::Point3D pos2 = Territory_Action::pos_to_tuple(parsed[1]);
      std::ostringstream cmd;
      cmd << "tty add_sub " << get<0>(pos1) << " " << get<1>(pos1) << " " << get<2>(pos1) << " " << get<0>(pos2) << " " << get<1>(pos2) << " " << get<2>(pos2);
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("错误的坐标!"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });

  player->sendForm(form);
}

//快速创建领地主菜单
void Menu::openQuickCreateMainMenu(endstone::Player*  player) const {
  endstone::ModalForm menu;
  menu.setTitle(LangTty.getLocal("§l快速创建领地"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择领地类型"));
  ttyDropdown.setOptions({LangTty.getLocal("普通领地"),LangTty.getLocal("子领地")});
  ttyDropdown.setDefaultIndex(0);
  menu.addControl(ttyDropdown);
  menu.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
   auto parse = nlohmann::json::parse(response);
    std::ostringstream cmd;
    if (parse[0] == 0) {
      cmd << "tty quick add";
      (void)p->performCommand(cmd.str());
    } else {
      cmd << "tty quick add_sub";
      (void)p->performCommand(cmd.str());
    }
  });
  menu.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });
  player->sendForm(menu);
}

// 重命名领地菜单
void Menu::openRenameTtyMenu(endstone::Player* player, const std::vector<std::string>& ttyNames) const {
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l重命名领地"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要重命名的领地"));
  ttyDropdown.setOptions(ttyNames);

  endstone::TextInput newNameInput;
  newNameInput.setLabel(LangTty.getLocal("§l新名字"));

  form.addControl(ttyDropdown);
  form.addControl(newNameInput);

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      if (parsed[1].empty()) {
        p->sendErrorMessage(LangTty.getLocal("输入不能为空"));
        return;
      }
      const int ttyIndex = parsed[0].get<int>();
      const string& oldname = ttyNames.at(ttyIndex);
      const string newname = parsed[1];
      std::ostringstream cmd;
      cmd << "tty rename \"" << oldname << "\" \"" << newname << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });

  player->sendForm(form);
}

// 传送自己及已加入的领地菜单
void Menu::openTpTtyMenu(endstone::Player* player) const {
  // 获取玩家成员及以上权限的领地列表
  const auto ttyList = Territory_Action::getMemberTtyNames(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }

  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("传送领地"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择你要传送的领地"));
  ttyDropdown.setOptions(ttyList);

  form.addControl(ttyDropdown);

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int ttyIndex = parsed[0].get<int>();
      const std::string& ttyname = ttyList.at(ttyIndex);
      std::ostringstream cmd;
      cmd << "tty tp \"" << ttyname << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });

  player->sendForm(form);
}

// 传送全部领地菜单
void Menu::openTpAllTtyMenu(endstone::Player* player) const {
  // 获取玩家全部可见领地列表
  auto ttyList = Territory_Action::getAllTtyNames();
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }

  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("传送全部领地"));

  endstone::TextInput InputTty;
  InputTty.setLabel(LangTty.getLocal("§l输入你要传送的领地"));

  form.addControl(InputTty);

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      if (parsed[0].empty()) {
        p->sendErrorMessage(LangTty.getLocal("输入不能为空"));
        return;
      }
      std::ostringstream cmd;
      cmd << "tty tp " << parsed[0] << "";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openMainMenu(p);
  });

  player->sendForm(form);
}

// 列出自己的全部领地
void Menu::openListTtyMenu(endstone::Player* player) {
  const auto ttyList = Territory_Action::getPlayerTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("你没有领地"));
    return;
  }

  std::ostringstream content;
  for (size_t i = 0; i < ttyList.size(); ++i) {
    const auto& tty = ttyList[i];
    content << LangTty.getLocal("§e") << (i + 1) << LangTty.getLocal(". 领地名称: ") << tty.name 
            << LangTty.getLocal("\n位置范围: ") << "\n" 
            << "(" << std::get<0>(tty.pos1) << ", " << std::get<1>(tty.pos1) << ", " << std::get<2>(tty.pos1) << ")"
            << LangTty.getLocal(" 到 ") 
            << "(" << std::get<0>(tty.pos2) << ", " << std::get<1>(tty.pos2) << ", " << std::get<2>(tty.pos2) << ")"
            << LangTty.getLocal("\n传送点位置: ") << "\n"
            << "(" << std::get<0>(tty.tppos) << ", " << std::get<1>(tty.tppos) << ", " << std::get<2>(tty.tppos) << ")"
            << LangTty.getLocal("\n维度: ") << tty.dim
            << LangTty.getLocal("\n是否允许玩家交互: ") << (tty.if_jiaohu ? "true" : "false")
            << LangTty.getLocal("\n是否允许玩家破坏: ") << (tty.if_break ? "true" : "false")
            << LangTty.getLocal("\n是否允许外人传送: ") << (tty.if_tp ? "true" : "false")
            << LangTty.getLocal("\n是否允许放置方块: ") << (tty.if_build ? "true" : "false")
            << LangTty.getLocal("\n是否允许实体爆炸: ") << (tty.if_bomb ? "true" : "false")
            << LangTty.getLocal("\n是否允许实体伤害: ") << (tty.if_damage ? "true" : "false")
            << LangTty.getLocal("\n领地管理员: ") << tty.manager
            << LangTty.getLocal("\n领地成员: ") << tty.member << "\n"
            << "--------------------\n";
  }

  endstone::ActionForm form;
  form.setTitle(LangTty.getLocal("§l领地列表"));
  form.setContent(content.str());
  
  player->sendForm(form);
}

// 管理自己管理的领地权限
void Menu::openSetPermisMenu(endstone::Player* player) const {
  auto ttyList = Territory_Action::getOpTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }

  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l领地权限管理界面"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要管理的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  form.addControl(ttyDropdown);

  form.setOnSubmit([this, ttyList](endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int index = parsed[0].get<int>();
      if (index < 0 || index >= static_cast<int>(ttyList.size())) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      openSetPermisDetailMenu(p, ttyList[index]);
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 领地权限详细设置
void Menu::openSetPermisDetailMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const {
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l管理领地") + " " + tty.name + LangTty.getLocal("的权限"));

  endstone::Toggle t_jiaohu, t_break, t_tp, t_build, t_bomb, t_damage;
  t_jiaohu.setLabel(LangTty.getLocal("§l是否允许外人领地内交互"));
  t_jiaohu.setDefaultValue(tty.if_jiaohu);
  t_break.setLabel(LangTty.getLocal("§l是否允许外人领地内破坏"));
  t_break.setDefaultValue(tty.if_break);
  t_tp.setLabel(LangTty.getLocal("§l是否允许外人传送至领地"));
  t_tp.setDefaultValue(tty.if_tp);
  t_build.setLabel(LangTty.getLocal("§l是否允许外人领地内放置"));
  t_build.setDefaultValue(tty.if_build);
  t_bomb.setLabel(LangTty.getLocal("§l是否允许领地内实体爆炸"));
  t_bomb.setDefaultValue(tty.if_bomb);
  t_damage.setLabel(LangTty.getLocal("§l是否允许外人对实体攻击"));
  t_damage.setDefaultValue(tty.if_damage);

  form.setControls({t_jiaohu, t_break, t_tp, t_build, t_bomb, t_damage});

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto permis = nlohmann::json::parse(response);
      if (!permis.is_array() || permis.size() != 6) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      std::vector<std::pair<std::string, bool>> fields = {
        {"if_jiaohu", permis[0].get<bool>()},
        {"if_break", permis[1].get<bool>()},
        {"if_tp", permis[2].get<bool>()},
        {"if_build", permis[3].get<bool>()},
        {"if_bomb", permis[4].get<bool>()},
        {"if_damage", permis[5].get<bool>()}
      };
      bool changed = false;
      for (size_t i = 0; i < fields.size(); ++i) {
        bool oldval = false;
        switch (i) {
          case 0: oldval = tty.if_jiaohu; break;
          case 1: oldval = tty.if_break; break;
          case 2: oldval = tty.if_tp; break;
          case 3: oldval = tty.if_build; break;
          case 4: oldval = tty.if_bomb; break;
          case 5: oldval = tty.if_damage; break;
          default: ;
        }
        if (fields[i].second != oldval) {
          std::ostringstream cmd;
          cmd << "tty set " << fields[i].first << " " << (fields[i].second ? "true" : "false") << " \"" << tty.name << "\"";
          (void)p->performCommand(cmd.str());
          changed = true;
        }
      }
      if (!changed) {
        p->sendErrorMessage(LangTty.getLocal("你未更改领地权限,领地权限不会变化"));
      }
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 删除自己管理的领地成员
void Menu::openDelTtyMemberMenu(endstone::Player* player) const {
  auto ttyList = Territory_Action::getOpTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l领地成员删除界面"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要删除成员的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  form.addControl(ttyDropdown);

  form.setOnSubmit([this, ttyList](endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int index = parsed[0].get<int>();
      if (index < 0 || index >= static_cast<int>(ttyList.size())) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      openDelTtyMemberSubMenu(p, ttyList[index]);
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 删除成员子菜单
void Menu::openDelTtyMemberSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const {
  std::vector<std::string> members = DataBase::splitString(tty.member);
  if (members.empty() || (members.size() == 1 && members[0].empty())) {
    player->sendErrorMessage(LangTty.getLocal("在领地") + tty.name + LangTty.getLocal("中没有任何成员"));
    return;
  }
  std::string ttyName = tty.name;
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l管理领地") + " " + tty.name + LangTty.getLocal("的成员"));

  endstone::Dropdown memberDropdown;
  memberDropdown.setLabel(LangTty.getLocal("选择要删除的领地成员"));
  memberDropdown.setOptions(members);

  form.addControl(memberDropdown);
  std::string player_name = player->getName();

  form.setOnSubmit([members, ttyName](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int idx = parsed[0].get<int>();
      if (idx < 0 || idx >= static_cast<int>(members.size())) {
        if (p) p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      std::ostringstream cmd;
      cmd << "tty member remove \"" << members[idx] << "\" \"" << ttyName << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      if (p) p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 添加自己管理的领地成员
void Menu::openAddTtyMemberMenu(endstone::Player* player) const {
  auto ttyList = Territory_Action::getOpTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l领地成员添加界面"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要添加成员的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  form.addControl(ttyDropdown);

  form.setOnSubmit([this, ttyList](endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int index = parsed[0].get<int>();
      if (index < 0 || index >= static_cast<int>(ttyList.size())) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      openAddTtyMemberSubMenu(p, ttyList[index]);
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 添加成员子菜单
void Menu::openAddTtyMemberSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const {
  const auto onlinePlayers = getOnlinePlayerList();
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l添加成员到领地") + " " + tty.name + LangTty.getLocal("中"));

  endstone::Dropdown onlineDropdown;
  onlineDropdown.setLabel(LangTty.getLocal("§l选择要添加的在线玩家"));
  onlineDropdown.setOptions(onlinePlayers);

  endstone::TextInput offlineInput;
  offlineInput.setLabel(LangTty.getLocal("§l添加不在线的玩家"));
  offlineInput.setPlaceholder(LangTty.getLocal("只要这里写了一个字都会以此为输入值"));

  form.setControls({onlineDropdown, offlineInput});

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      int idx = parsed[0].get<int>();
      if (std::string leave_player = parsed[1]; !leave_player.empty()) {
        std::ostringstream cmd;
        cmd << "tty member add \"" << leave_player << "\" \"" << tty.name << "\"";
        (void)p->performCommand(cmd.str());
      } else if (idx >= 0 && idx < static_cast<int>(onlinePlayers.size())) {
        std::ostringstream cmd;
        cmd << "tty member add \"" << onlinePlayers[idx] << "\" \"" << tty.name << "\"";
        (void)p->performCommand(cmd.str());
      } else {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
      }
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 删除自己领地的领地管理员
void Menu::openDelTtyManagerMenu(endstone::Player* player) const {
  auto ttyList = Territory_Action::getPlayerTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l领地管理员删除界面"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要删除领地管理员的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  form.addControl(ttyDropdown);

  form.setOnSubmit([this, ttyList](endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      int index = parsed[0].get<int>();
      if (index < 0 || index >= static_cast<int>(ttyList.size())) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      openDelTtyManagerSubMenu(p, ttyList[index]);
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 删除管理员子菜单
void Menu::openDelTtyManagerSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const {
  std::vector<std::string> managers = DataBase::splitString(tty.manager);
  if (managers.empty() || (managers.size() == 1 && managers[0].empty())) {
    player->sendErrorMessage(LangTty.getLocal("在领地") + tty.name + LangTty.getLocal("中没有任何领地管理员"));
    return;
  }
  std::string ttyName = tty.name;
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l管理领地") + " " + tty.name + LangTty.getLocal("的领地管理员"));

  endstone::Dropdown managerDropdown;
  managerDropdown.setLabel(LangTty.getLocal("选择要删除的领地管理员"));
  managerDropdown.setOptions(managers);

  form.addControl(managerDropdown);

  form.setOnSubmit([managers,ttyName](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int idx = parsed[0];
      std::ostringstream cmd;
      cmd << "tty manager remove \"" << managers[idx] << "\" \"" << ttyName << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 添加自己领地的领地管理员
void Menu::openAddTtyManagerMenu(endstone::Player* player) const {
  auto ttyList = Territory_Action::getPlayerTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l领地管理员添加界面"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要添加领地管理员的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  form.addControl(ttyDropdown);

  form.setOnSubmit([this, ttyList](endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      int index = parsed[0].get<int>();
      if (index < 0 || index >= static_cast<int>(ttyList.size())) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      openAddTtyManagerSubMenu(p, ttyList[index]);
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 添加管理员子菜单
void Menu::openAddTtyManagerSubMenu(endstone::Player* player, const Territory_Action::TerritoryData& tty) const {
  const auto onlinePlayers = getOnlinePlayerList();
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l添加领地管理员到领地") + " " + tty.name + LangTty.getLocal("中"));

  endstone::Label infoLabel;
  infoLabel.setText(LangTty.getLocal("§l领地管理员有领地权限设置、成员管理、领地传送点设置、创建子领地的权限,请把握好人选"));

  endstone::Dropdown onlineDropdown;
  onlineDropdown.setLabel(LangTty.getLocal("§l选择要添加的在线玩家"));
  onlineDropdown.setOptions(onlinePlayers);

  endstone::TextInput offlineInput;
  offlineInput.setLabel(LangTty.getLocal("§l添加不在线的玩家"));
  offlineInput.setPlaceholder(LangTty.getLocal("只要这里写了一个字都会以此为输入值"));

  form.setControls({infoLabel, onlineDropdown, offlineInput});

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      int idx = parsed[1].get<int>();
      std::string leave_player = parsed[2];
      if (!leave_player.empty()) {
        std::ostringstream cmd;
        cmd << "tty manager add \"" << leave_player << "\" \"" << tty.name << "\"";
        (void)p->performCommand(cmd.str());
      } else if (idx >= 0 && idx < static_cast<int>(onlinePlayers.size())) {
        std::ostringstream cmd;
        cmd << "tty manager add \"" << onlinePlayers[idx] << "\" \"" << tty.name << "\"";
        (void)p->performCommand(cmd.str());
      } else {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
      }
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 设置自己管理的领地的传送点
void Menu::openSetTpTtyMenu(endstone::Player* player) const {
  const auto ttyList = Territory_Action::getOpTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l设置领地传送点"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要设置传送点的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  endstone::TextInput tpInput;
  tpInput.setLabel(LangTty.getLocal("§l坐标"));
  std::ostringstream playerPos;
  playerPos << player->getLocation().getX() << " " << player->getLocation().getY() << " " << player->getLocation().getZ();
  tpInput.setDefaultValue(playerPos.str());

  form.setControls({ttyDropdown, tpInput});

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int index = parsed[0].get<int>();
      const std::string tppos = parsed[1];
      if (index < 0 || index >= static_cast<int>(ttyList.size()) || tppos.empty()) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      std::ostringstream cmd;
      cmd << "tty settp " << tppos << " \"" << ttyList[index].name << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 转让领地菜单
void Menu::openTransferTtyMenu(endstone::Player* player) const {
  const auto ttyList = Territory_Action::getPlayerTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l转让领地"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要转让的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  endstone::TextInput newOwnerInput;
  newOwnerInput.setLabel(LangTty.getLocal("§l接收领地的玩家名"));

  form.setControls({ttyDropdown, newOwnerInput});

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int index = parsed[0].get<int>();
      const std::string newOwner = parsed[1];
      if (index < 0 || index >= static_cast<int>(ttyList.size()) || newOwner.empty()) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      std::ostringstream cmd;
      cmd << "tty transfer \"" << ttyList[index].name << "\" \"" << newOwner << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

// 删除领地菜单
void Menu::openDelTtyMenu(endstone::Player* player) const {
  const auto ttyList = Territory_Action::getPlayerTtyList(player->getName());
  if (ttyList.empty()) {
    player->sendErrorMessage(LangTty.getLocal("未查找到领地"));
    return;
  }
  endstone::ModalForm form;
  form.setTitle(LangTty.getLocal("§l删除领地"));

  endstone::Dropdown ttyDropdown;
  ttyDropdown.setLabel(LangTty.getLocal("§l选择要删除的领地"));
  std::vector<std::string> ttyNames;
  ttyNames.reserve(ttyList.size());
  for (const auto& tty : ttyList) ttyNames.push_back(tty.name);
  ttyDropdown.setOptions(ttyNames);

  endstone::Label warnLabel;
  warnLabel.setText(LangTty.getLocal("§l§4删除领地不可恢复!"));

  form.setControls({ttyDropdown, warnLabel});

  form.setOnSubmit([=](const endstone::Player* p, const std::string& response) {
    try {
      auto parsed = nlohmann::json::parse(response);
      const int index = parsed[0].get<int>();
      if (index < 0 || index >= static_cast<int>(ttyList.size())) {
        p->sendErrorMessage(LangTty.getLocal("未知的错误"));
        return;
      }
      std::ostringstream cmd;
      cmd << "tty del \"" << ttyList[index].name << "\"";
      (void)p->performCommand(cmd.str());
    } catch (...) {
      p->sendErrorMessage(LangTty.getLocal("未知的错误"));
    }
  });

  form.setOnClose([this](endstone::Player* p) {
    openManTtyMenu(p);
  });

  player->sendForm(form);
}

//快速创建领地菜单
void Menu::openQuickCreateTtyMenu(endstone::Player* player,Territory_Action::QuickTtyData& quick_tty_data) {
  if (quick_tty_data.dim1.empty() ||
      quick_tty_data.dim2.empty() ||
      quick_tty_data.player_name.empty() ||
      quick_tty_data.tty_type.empty()) {
    player->sendErrorMessage(LangTty.getLocal("领地数据不完整，无法创建领地"));
    return;
  }
  endstone::ModalForm menu;
  menu.setTitle(LangTty.getLocal("§l快速创建领地"));
  endstone::Label pos_info;
  pos_info.setText(to_string(get<0>(quick_tty_data.pos1)) + " y1 " + to_string(get<2>(quick_tty_data.pos1)) + "\n" +
    to_string(get<0>(quick_tty_data.pos2)) + " y2 " + to_string(get<2>(quick_tty_data.pos2))
    );
  endstone::Slider MaxY,MinY;
  MaxY.setLabel(LangTty.getLocal("§l设置领地最高点"));
  MaxY.setMax(320);MaxY.setMin(-64);MaxY.setDefaultValue(320);MaxY.setStep(1);

  MinY.setLabel(LangTty.getLocal("§l设置领地最低点"));
  MinY.setMin(-64);MinY.setMin(-64);MinY.setDefaultValue(-64);MinY.setStep(1);
  menu.setControls({MaxY,MinY,pos_info});
  bool create_status = false;
  menu.setOnSubmit([create_status,quick_tty_data](const endstone::Player* p, const std::string& response)mutable {
    auto parse = nlohmann::json::parse(response);
    const int maxY = parse[0];
    const int minY = parse[1];
    std::ostringstream cmd;
    if (quick_tty_data.tty_type == "tty") {
      cmd << "tty add " << get<0>(quick_tty_data.pos1) << " " << maxY << " " << get<2>(quick_tty_data.pos1) << " " << get<0>(quick_tty_data.pos2) << " " << minY << " " << get<2>(quick_tty_data.pos2);
    } else {
      cmd << "tty add_sub " << get<0>(quick_tty_data.pos1) << " " << maxY << " " << get<2>(quick_tty_data.pos1) << " " << get<0>(quick_tty_data.pos2) << " " << minY << " " << get<2>(quick_tty_data.pos2);
    }
    (void)p->performCommand(cmd.str());
    create_status = true;
  });
  menu.setOnClose([create_status](const endstone::Player* p) {
    if (!create_status) p->sendMessage(LangTty.getLocal("创建已取消"));
  });
  player->sendForm(menu);
}