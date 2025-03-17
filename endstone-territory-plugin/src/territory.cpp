//
// Created by yuhang on 2025/3/6.
//
#include "territory.h"

ENDSTONE_PLUGIN("territory", "0.2.1", Territory)
{
    description = "a territory plugin for endstone with C++";
    website = "https://github.com/yuhangle/endstone-territory";
    authors = {"yuhang2006 <yuhang2006@hotmail.com>"};

    command("tty")
            .description("Territory command")
            .usages("/tty (add)[opt: opt_add] [pos: pos] [pos: pos]",
                    "/tty (add_sub)[opt: opt_addsub] [pos: pos] [pos: pos]",
                    "/tty (list)[opt: opt_list]",
                    "/tty (del)[opt: opt_del] [msg: message]",
                    "/tty (rename)[opt: opt_rename] [msg: message] [msg: message]",
                    "/tty (set)<opt: opt_setper> (if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage)<opt: opt_permission> <bool: bool> <msg: message>",
                    "/tty (member)<opt: opt_member> (add|remove)<opt: opt_mem> <player: target> <msg: message>",
                    "/tty (manager)<opt: opt_manager> (add|remove)<opt: opt_man> <player: target> <msg: message>",
                    "/tty (settp)<opt: opt_settp> [pos: pos] <msg: message>",
                    "/tty (transfer)<opt: opt_transfer> <msg: message> <msg: message>",
                    "/tty (tp)<opt: opt_tp> <msg: message>",
                    "/tty (help)<opt: opt_help>"
                    )
            .permissions("territory.command.member");

    command("optty")
            .description("Territory op command")
            .usages("/optty (del)[opt: opt_op_del] [msg: message]",
                    "/optty (del_all)[opt: opt_op_del_all] <player: target>",
                    "/optty (set)<opt: opt_op_setper> (if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage)<opt: opt_op_permission> <bool: bool> <msg: message>",
                    "/optty (reload)[opt: opt_reloadtty]"
            )
            .permissions("territory.command.op");

    permission("territory.command.member")
            .description("member command")
            .default_(endstone::PermissionDefault::True);

    permission("territory.command.op")
            .description("op command")
            .default_(endstone::PermissionDefault::Operator);
}