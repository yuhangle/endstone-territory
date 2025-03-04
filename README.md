# Territory领地插件

## 介绍

Territory领地插件是使用Python开发的运行在endstone插件加载器上的三维领地插件,不依赖经济类插件,玩家可免费创建领地；使用sqlite数据库储存领地数据,支持防交互、防破坏方块、防放置方块、实体爆炸、领地传送、成员管理、添加领地管理员等

Territory插件支持子领地,子领地可由父领地主人和父领地管理员在父领地内创建,子领地权限和人员与父领地相互独立,子领地受子领地所有者完全控制,不受父领地控制,父领地被删除后,子领地失去父领地标签成为独立领地,不会随父领地一并删除

## 如何使用

> 安装&配置

**安装Endstone**

此步请查看endstone文档

**下载&安装Territory插件**

前往Releases处下载最新版本的插件whl文件,然后放在服务端目录的plugins文件夹里

**配置**

首次运行插件后将自动在plugins目录创建territoty文件夹,里面包含配置文件config.json和领地数据库文件territory_data.db
配置文件的默认配置如下:
```bash
{
    "player_max_tty_num": 20
}
```
player_max_tty_num 为玩家可拥有的领地的最大值,默认为20个  
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
/tty set 权限名 权限值 领地名
```

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
/optty delall 玩家名
```

重载领地数据和配置

```bash
/reloadtty
```