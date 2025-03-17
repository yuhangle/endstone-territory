# Territory领地插件

> 插件尚在开发中，欢迎反馈bug

 [English](README.md)

## 介绍

Territory领地插件是使用C++开发的运行在endstone插件加载器上的三维领地插件,不依赖经济类插件,玩家可免费创建领地；使用sqlite数据库储存领地数据,支持防交互、防破坏方块、防放置方块、防实体爆炸、防外人对领地内实体一般伤害、领地传送、成员管理、添加领地管理员等。你可以使用此插件在endstone服务器上保护玩家们的建筑和财产。

Territory插件支持子领地,子领地可由父领地主人和父领地管理员在父领地内创建,子领地权限和人员与父领地相互独立,子领地受子领地所有者完全控制,不受父领地控制,父领地被删除后,子领地失去父领地标签成为独立领地,不会随父领地一并删除

## 特点

Territory插件本体在0.1.0版本后使用C++开发，性能更好，响应更快；使用SQlite储存领地数据，无需额外配置，开箱即用。

## 如何使用

> 安装&配置

插件分为图形菜单与插件本体两个插件，图形菜单插件使用Python编写，不具备本体功能；插件本体无需图形菜单插件即可运行。

**安装Endstone**

此步请查看endstone文档

**下载&安装Territory插件**

> Windows平台

前往Releases处下载最新版本的插件本体dll文件和领地菜单插件whl文件,然后放在服务端目录的plugins文件夹里

> Linux平台

前往Releases处下载最新版本的插件本体so文件和领地菜单插件whl文件,然后放在服务端目录的plugins文件夹里

**配置**

首次运行插件后将自动在plugins目录创建territoty文件夹,里面包含配置文件config.json和领地数据库文件territory_data.db
配置文件的默认配置如下:

```bash
{
    "player_max_tty_num": 20,
    "actor_fire_attack_protect": true
}
```

player_max_tty_num 为玩家可拥有的领地的最大值,默认为20个

actor_fire_attack_protect 为是否开启生物火焰保护,默认开启;由于玩家对生物的直接攻击插件可以拦截，但是当玩家武器存在火焰附加附魔时,附魔效果依然会作用在生物身上造成杀伤导致保护不全;配置文件中开启生物火焰保护后,无权限玩家将无法对领地内生物造成包括火焰附加在内的任何伤害,但同时生物也将免疫部分火焰伤害

> 命令用法和领地使用管理

**命令列表**

打开领地菜单

```shell
/ttygui
```

新建领地

```shell
/tty add 领地边角坐标1 领地边角坐标2
```

新建子领地

```shell
/tty add_sub 子领地边角坐标1 子领地边角坐标2
```

列出领地

```shell
/tty list
```

删除领地

```shell
/tty del 领地名
```

重命名领地

```shell
/tty rename 旧领地名 新领地名
```

设置领地权限

```shell
/tty set 权限名(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage) 权限值 领地名
```

其中权限名分别代表:是否允许外人领地内交互、是否允许外人领地内破坏、是否允许外人传送至领地、是否允许外人领地内放置、是否允许领地内实体爆炸、是否允许外人对实体攻击

设置领地管理员

```shell
/tty manager add|remove(添加|删除) 玩家名 领地名
```

设置领地成员

```shell
/tty member add|remove(添加|删除) 玩家名 领地名
```

设置领地传送点

```bash
/tty settp 领地传送坐标 领地名
```

传送领地

```bash
/tty tp 领地名
```

**管理员命令**

删除领地

```bash
/optty del 领地名
```

删除玩家的全部领地

```bash
/optty del_all 玩家名
```

设置玩家的领地权限

```bash
/optty set 权限名(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage) 权限值 领地名
```

重载领地数据和配置

```bash
/optty reload
```
