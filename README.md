>*此插件正在开发中*

# Territory领地插件

## 介绍

Territory领地插件是使用Python开发的运行在endstone插件加载器上的三维领地插件,不依赖经济类插件,玩家可免费创建领地；使用sqlite数据库储存领地数据,支持防交互、防破坏方块、防放置方块、实体爆炸、领地传送、成员管理、添加领地管理员等

## 如何使用

> 安装&配置

**安装Endstone**

此步请查看endstone文档

**下载&安装Territory插件**

前往Releases处下载最新版本的插件whl文件,然后放在服务端目录的plugins文件夹里

> 命令用法和领地使用管理

**命令列表**

新建领地
`/tty add 领地边角坐标1 领地边角坐标2`

列出领地
`/tty list`

删除领地
`/tty del 领地名`

重命名领地
`/tty rename 旧领地名 新领地名`

设置领地权限
`/tty set 权限名 权限值 领地名`

设置领地管理员
`/tty manager add|remove(添加|删除) 玩家名 领地名`

设置领地成员
`/tty member add|remove(添加|删除) 玩家名 领地名`

设置领地传送点
`/tty settp 领地传送坐标 领地名`

传送领地
`/tty tp 领地名`