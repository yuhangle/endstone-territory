![header](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Territory%20Plugin&textBg=false&desc=About%20A%20three-dimensional%20territory%20plugin%20developed%20for%20Endstone%20&descAlignY=70&fontColor=802e82&reversal=false)

 [![English](https://img.shields.io/badge/English-README_eng.md-blue)](README.md)
 [![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
 ![Build Status](https://github.com/yuhangle/endstone-territory/actions/workflows/build.yml/badge.svg)

## 介绍

Territory领地插件是使用C++开发的运行在endstone插件加载器上的三维领地插件；使用sqlite数据库储存领地数据,支持防交互、防破坏方块、防放置方块、防实体爆炸、防外人对领地内实体一般伤害、领地传送、成员管理、添加领地管理员等。你可以使用此插件在endstone服务器上保护玩家们的建筑和财产。

Territory领地插件支持子领地,子领地可由父领地主人和父领地管理员在父领地内创建,子领地权限和人员与父领地相互独立,子领地受子领地所有者完全控制,不受父领地控制,父领地被删除后,子领地失去父领地标签成为独立领地,不会随父领地一并删除。

Territory领地插件支持umoney插件作为经济系统接入,默认关闭经济。在配置文件中开启经济后,创建领地会向玩家收取领地面积x单价的费用，删除领地即可以当前价格退款。

## 特点

Territory插件使用C++开发，使用SQlite储存领地数据，理论性能优秀。

### ⚡ 性能极致优化

#### 🔧 架构级优化
- **纯 C++ 编写**：零解释器开销，极致执行效率
- **双模存储引擎**：SQLite 持久化 + 内存缓存，兼顾启动速度、查询稳定性与崩溃安全

#### 🧠 算法与数据结构优化
- **空间网格索引（Spatial Grid）**：将 O(n) 全表扫描优化为 O(1) 近似常数查找
- **权限缓存**：加载或修改领地时一次性构建哈希集合，权限校验从 O(k) 字符串分割降为 O(1) 哈希查找

#### 📊 百万级负载实测数据
- 交互事件平均耗时：**36.8 μs**
- 位置查询（完整）平均耗时：**125 μs**
- 百万领地下，单次交互稳定控制在 **40 μs 以内**（约 0.04 毫秒）


### 📊 500个随机领地负载下的性能测试

> 测试环境：**500个随机领地负载**，测试记录的是单次事件或定期执行函数的完整运行耗时


| 排名 | 插件名称 | 领地内交互 | 领地外交互 | 位置信息发送（完整处理） | 综合评价 |
|:----:|:--------:|----------:|----------:|------------------------:|:--------|
| 🥇 | Territory | 12.47 | 11.38 | 39.83 | 🚀 **极优**，高效且轻量 |
| 🥈 | 领地U | 89.37 | 106.18 | 372.58 | ✅ **良好**，适合中小服 |
| 🥉 | 领地E | 6,949.44 | 1,093.68 | 1,138.25 | ❌ **高延迟**，不推荐高负载 |

> **数据解读**
> - **领地内交互**：玩家在领地内部交互时，对应函数的平均耗时
> - **领地外交互**：玩家在领地外部交互时，对应函数的平均耗时
> - **位置信息发送**：完整处理玩家位置变动、发送领地信息时，对应函数的平均耗时
> - **时间单位均为 微秒(μs)**


## 如何使用

> 安装&配置

**安装Endstone**

此步请查看endstone文档

**下载&安装Territory插件**

前往Releases处下载最新版本的插件压缩包,然后解压到服务端目录的plugins文件夹里，目录结构如下:

Windows:
```
plugins/
├── endstone_territory.dll    # Windows插件文件
└── territory/                # 数据目录
    └── language/             # 语言文件夹
```

Linux:
```
plugins/
├── endstone_territory.so     # Linux插件文件
└── territory/                # 数据目录
    └── language/             # 语言文件夹
```

---

**配置**

首次运行插件后将自动在plugins目录创建territory文件夹,里面包含配置文件config.json和领地数据库文件territory_data.db
配置文件的默认配置如下:

```json
{
    "actor_fire_attack_protect": true,
    "language": "zh_CN",
    "max_tty_area": 4000000,
    "money_with_umoney": false,
    "player_max_tty_num": 20,
    "price": 1,
    "allow_fly_on_territory": false
}
```
`language` 为插件使用的语言，默认为中文，可根据语言文件夹内的语言文件更改配置。

`actor_fire_attack_protect` 为是否开启生物火焰保护,默认开启;由于玩家对生物的直接攻击插件可以拦截，但是当玩家武器存在火焰附加附魔时,附魔效果依然会作用在生物身上造成杀伤导致保护不全;配置文件中开启生物火焰保护后,无权限玩家将无法对领地内生物造成包括火焰附加在内的任何伤害,但同时生物也将免疫部分火焰伤害。

`max_tty_area` 为玩家创建领地的最大面积，默认为4000000单位（2000x2000）,领地超过此面积将无法创建。

`money_with_umoney` 为是否启用umoney插件作为经济系统，默认关闭。开启后创建领地将通过umoney插件的经济系统向玩家收取费用。

`player_max_tty_num` 为玩家可拥有的领地的最大值,默认为20个。

`price` 为领地单位面积价格，默认为1。开启经济后，其值作为领地单位面积价格用于计算领地总价。

`allow_fly_on_territory` 为是否允许玩家在领地内飞行，默认关闭。

---

> 命令用法和领地使用管理

**命令列表**

打开领地菜单

```shell
/tty
```

---

新建领地

```shell
/tty add 领地边角坐标1 领地边角坐标2
```

---

新建子领地

```shell
/tty add_sub 子领地边角坐标1 子领地边角坐标2
```

---

打开快速创建领地菜单

```shell
/tty quick add
```

---

打开快速创建子领地菜单

```shell
/tty quick add_sub
```

---

列出领地

```shell
/tty list
```

---

删除领地

```shell
/tty del 领地名
```

---

重命名领地

```shell
/tty rename 旧领地名 新领地名
```

---

设置领地权限

```shell
/tty set 权限名(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage|if_edge_piston|if_wither) 权限值 领地名
```

其中权限名分别代表:是否允许外人领地内交互、是否允许外人领地内破坏、是否允许外人传送至领地、是否允许外人领地内放置、是否允许领地内实体爆炸、是否允许外人对实体攻击、是否允许领地边缘活塞工作、是否允许领地内凋零活动。

---

设置领地管理员

```shell
/tty manager add|remove(添加|删除) 玩家名 领地名
```

---

设置领地成员

```shell
/tty member add|remove(添加|删除) 玩家名 领地名
```

---

设置领地传送点

```shell
/tty settp 领地传送坐标 领地名
```

---

传送领地

```shell
/tty tp 领地名
```

---

修改领地大小

```shell
/tty resize 领地名 新坐标1 新坐标2
```

---

**管理员命令**

删除领地

```shell
/optty del 领地名
```

---

删除玩家的全部领地

```shell
/optty del_all 玩家名
```

---

设置玩家的领地权限

```shell
/optty set 权限名(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage|if_edge_piston|if_wither) 权限值 领地名
```

---

转让一个领地给其他玩家

```shell
/optty transfer 领地名 玩家名
```

---

转让一个玩家的所有领地给另一个玩家

```shell
/optty transfer_all 玩家名 玩家名
```

---

重载领地数据和配置

```shell
/optty reload
```

---

## 📦 使用的项目 & 鸣谢

本插件基于以下开源项目构建，衷心感谢各项目的作者与维护者：

- **[nlohmann/json](https://github.com/nlohmann/json)**  
  高性能、易用的 JSON 解析与序列化库

- **[fmtlib/fmt](https://github.com/fmtlib/fmt)**  
  现代化的字符串格式化库，提供安全高效的输出能力

- **[GlacieTeam/BinaryStream](https://github.com/GlacieTeam/BinaryStream)**  
  二进制流解析库，用于处理数据包
  