![header](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Territory%20Plugin&textBg=false&desc=About%20A%20three-dimensional%20territory%20plugin%20developed%20for%20Endstone%20&descAlignY=70&fontColor=802e82&reversal=false)

> 插件尚在开发中，欢迎反馈bug

 [English](README.md)

## 介绍

Territory领地插件是使用C++开发的运行在endstone插件加载器上的三维领地插件；使用sqlite数据库储存领地数据,支持防交互、防破坏方块、防放置方块、防实体爆炸、防外人对领地内实体一般伤害、领地传送、成员管理、添加领地管理员等。你可以使用此插件在endstone服务器上保护玩家们的建筑和财产。

Territory领地插件支持子领地,子领地可由父领地主人和父领地管理员在父领地内创建,子领地权限和人员与父领地相互独立,子领地受子领地所有者完全控制,不受父领地控制,父领地被删除后,子领地失去父领地标签成为独立领地,不会随父领地一并删除

Territory领地插件支持umoney插件作为经济系统接入,默认关闭经济。在配置文件中开启经济后,创建领地会向玩家收取领地面积x单价的费用，删除领地即可以当前价格退款。

## 特点

Territory插件使用C++开发，使用SQlite储存领地数据，理论性能更好。

## 如何使用

> 安装&配置

**安装Endstone**

此步请查看endstone文档

**下载&安装Territory插件**

前往Releases处下载最新版本的插件压缩包,然后解压到服务端目录的plugins文件夹里，目录结构如下:

Windows:
- plugins/
    - endstone_territory.dll # Windows插件文件
    - territory/ # 数据目录
        - language # 语言文件夹

Linux:
- plugins/
    - endstone_territory.so # Linux插件文件
    - territory/ # 数据目录
        - language # 语言文件夹

**配置**

首次运行插件后将自动在plugins目录创建territoty文件夹,里面包含配置文件config.json和领地数据库文件territory_data.db
配置文件的默认配置如下:

```json
{
    "actor_fire_attack_protect": true,
    "language": "zh_CN",
    "max_tty_area": 4000000,
    "money_with_umoney": false,
    "player_max_tty_num": 20,
    "price": 1
}
```
`language` 为插件使用的语言，默认为中文，可根据语言文件夹内的语言文件更改配置。

`actor_fire_attack_protect` 为是否开启生物火焰保护,默认开启;由于玩家对生物的直接攻击插件可以拦截，但是当玩家武器存在火焰附加附魔时,附魔效果依然会作用在生物身上造成杀伤导致保护不全;配置文件中开启生物火焰保护后,无权限玩家将无法对领地内生物造成包括火焰附加在内的任何伤害,但同时生物也将免疫部分火焰伤害

`max_tty_area` 为玩家创建领地的最大面积，默认为4000000单位（2000x2000）,领地超过此面积将无法创建。

`money_with_umoney` 为是否启用umoney插件作为经济系统，默认关闭。开启后创建领地将通过umoney插件的经济系统向玩家收取费用。

`player_max_tty_num` 为玩家可拥有的领地的最大值,默认为20个

`price` 为领地单位面积价格，默认为1。开启经济后，其值作为领地单位面积价格用于计算领地总价

> 命令用法和领地使用管理

**命令列表**

打开领地菜单

```shell
/tty
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

修改领地大小

```bash
/tty resize 领地名 新坐标1 新坐标2
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
