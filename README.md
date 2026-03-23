![header](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Territory%20Plugin&textBg=false&desc=About%20A%20three-dimensional%20territory%20plugin%20developed%20for%20Endstone%20&descAlignY=70&fontColor=802e82&reversal=false)

[![简体中文](https://img.shields.io/badge/Chinese-README_chsinese.md-blue)](README_zh-CN.md)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
![Build Status](https://github.com/yuhangle/endstone-territory/actions/workflows/build.yml/badge.svg)

## Introduction

The Territory plugin is a 3D territory management plugin developed in C++ for the endstone plugin loader. It uses an SQLite database to store territory data and supports features such as anti-interaction, anti-block breaking, anti-block placement, anti-entity explosion, preventing general damage to entities within the territory from outsiders, territory teleportation, member management, and adding territory administrators. You can use it to protect players' buildings and property on endstone server.

The Territory plugin supports sub-territories, which can be created within a parent territory by the parent territory owner and administrators. Sub-territory permissions and members are independent from the parent territory. Sub-territories are fully controlled by their owners and are not controlled by the parent territory. If a parent territory is deleted, its sub-territories will lose their parent territory tag and become independent territories, and will not be deleted along with the parent territory.

The Territory plugin supports the integration of the umoney plugin as an economic system, with the economy being disabled by default. After enabling the economy in the configuration file, creating a territory will charge players based on the territory's area multiplied by the unit price. Deleting a territory will refund the player at the current price.

## Features

The Territory plugin is developed in C++ and uses SQLite for land data storage, delivering excellent theoretical performance.

### ⚡ Extreme Performance Optimization

#### 🔧 Architecture-Level Optimization
- **Pure C++ Implementation**: Zero interpreter overhead for great execution efficiency
- **Dual-Mode Storage Engine**: SQLite persistence + in-memory cache, balancing startup speed, query stability, and crash safety

#### 🧠 Algorithm & Data Structure Optimization
- **Spatial Grid Index**: Reduces O(n) full table scans to O(1) approximate constant-time lookups
- **Permission Cache**: Builds a hash set once when loading or modifying land data, reducing permission checks from O(k) string splitting to O(1) hash lookups

#### 📊 Real-World Test Data Under Million-Scale Load
- Average interaction event latency: **36.8 μs**
- Average location query (full processing) latency: **125 μs**
- Under a million land claims, single interaction latency is consistently below **40 μs** (approximately 0.04 milliseconds)

### 📊 Performance Test Under 500 Random Land Claims

> Test Environment: **500 random land claims**. The recorded values represent the total execution time of a single event or periodically executed function.

| Rank | Plugin Name | Interaction Inside Land | Interaction Outside Land | Location Info Send (Full Processing) | Overall Assessment |
|:----:|:-----------:|------------------------:|-------------------------:|-------------------------------------:|:------------------|
| 🥇 | Territory | 12.47 | 11.38 | 39.83 | 🚀 **Excellent**, highly efficient and lightweight |
| 🥈 | Land of U | 89.37 | 106.18 | 372.58 | ✅ **Good**, suitable for small to medium servers |
| 🥉 | Land of E | 6,949.44 | 1,093.68 | 1,138.25 | ❌ **High latency**, not recommended for heavy loads |

> **Data Interpretation**
> - **Interaction Inside Land**: Average latency of the corresponding function when a player interacts inside a land claim.
> - **Interaction Outside Land**: Average latency of the corresponding function when a player interacts outside any land claim.
> - **Location Info Send**: Average latency of the corresponding function when fully processing player movement and sending land information.
> - **All time units are in microseconds (μs).**


## How to use

> Install & Config

**Install Endstone**

Please refer to the endstone documentation for this step.

**Download & Install Territory Plugin**

Go to the Releases page to download the latest version of the plugin archive, then extract it into the plugins folder in your server directory. The directory structure is as follows:

Windows:
- plugins/
    - endstone_territory.dll # Plugin file for Windows
    - territory/ # Data directory
        - language # Language directory

Linux:
- plugins/
    - endstone_territory.so # Plugin file for Linux
    - territory/ # Data directory
        - language # Language directory

**Language**

The plugin defaults to Chinese. You can change the language by replacing the `lang.json` file inside the plugin's data directory (`territory/`). The GitHub Releases page provides pre-packaged archives with different language versions — currently, Chinese and English are supported. You may also manually edit `lang.json` to adapt it to your preferred language.

**Config**

After running the plugin for the first time, a "territory" folder will be automatically created in the plugins directory. This folder contains the configuration file "config.json" and the territory database file "territory_data.db".

The default configuration of the configuration file is as follows:

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
`language` is the language used by the plugin, which is Chinese by default. You can change the configuration according to the language file in the language folder.

`actor_fire_attack_protect` indicates whether to enable creature fire protection, which is enabled by default. While the plugin can intercept direct attacks from players on creatures, if a player's weapon has a fire aspect enchantment, the enchantment effect will still apply to the creatures, causing damage and resulting in incomplete protection. After enabling creature fire protection in the configuration file, unauthorized players will be unable to inflict any damage, including fire aspect damage, on creatures within the territory. However, creatures will also gain immunity to some fire damage.

`max_tty_area` represents the maximum area for a player-created territory, with a default of 4,000,000 units (2000x2000). Territories exceeding this size cannot be created.

`money_with_umoney` indicates whether to use the umoney plugin as the economy system, which is disabled by default. When enabled, creating territories will incur charges to the player through the economic system provided by the umoney plugin.

`player_max_tty_num` is the maximum number of territories a player can own, with a default value of 20.

`price` is the price per unit area of territory, with a default value of 1. When the economy is enabled, this value serves as the price per unit area of territory for calculating the total territory cost.

`allow_fly_on_territory` indicates whether players are allowed to fly within the territory. This is disabled by default.

> Command Usage and Territory Management

**Command list**

Open territory menu

```shell
/tty
```

Create New Territory

```shell
/tty add Territory-corner-coordinate-1 Territory-corner-coordinate-2
```

Create New Sub-Territory

```shell
/tty add_sub Sub-Territory-corner-coordinate-1 Sub-Territory-corner-coordinate-2
```

Open quick territory creation menu

```shell
/tty quick add
```

Open quick sub-territory creation menu

```shell
/tty quick add_sub
```

List Territory

```shell
/tty list
```

Delete Territory

```shell
/tty del territory-name
```

Rename Territory

```shell
/tty rename old-name new-name
```

Set Territory Permissions

```shell
/tty set permission(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage|if_edge_piston|if_wither) permission-value territory-name
```

Where the permission names respectively represent:
- Whether to allow outsiders to interact within the territory.
- Whether to allow outsiders to destroy within the territory.
- Whether to allow outsiders to teleport to the territory.
- Whether to allow outsiders to place within the territory.
- Whether to allow entity explosions within the territory.
- Whether to allow outsiders to attack entities.
- Whether to allow pistons to work at the territory edge.
- Whether to allow wither activities within the territory.

Set Territory Administrators

```shell
/tty manager add|remove player-name territory-name
```

Set Territory Members

```shell
/tty member add|remove player-name territory-name
```

Set Territory Teleport Point

```bash
/tty settp Territory-teleport-coordinates Territory-name
```

Teleport to Territory

```bash
/tty tp territory-name
```

Resize Territory

```bash
/tty resize territory-name new-coordinate-1 new-coordinate-2
```

**Administrator Commands**

Delete Territory

```bash
/optty del territory-name
```

Delete All Territories of a Player

```bash
/optty del_all player-name
```

Set Territory Permissions for a Player

```bash
/optty set permission(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage|if_edge_piston|if_wither) permission-value territory-name
```

Reload Territory Data and Configuration

```bash
/optty reload
```