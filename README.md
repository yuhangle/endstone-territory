![header](https://capsule-render.vercel.app/api?type=waving&height=300&color=gradient&text=Territory%20Plugin&textBg=false&desc=About%20A%20three-dimensional%20territory%20plugin%20developed%20for%20Endstone%20&descAlignY=70&fontColor=802e82&reversal=false)

> The plugin is still under development, feedback is welcome.

[简体中文](README_zh-CN.md)

## Introduction

The Territory plugin is a 3D territory management plugin developed in C++ for the endstone plugin loader.  It uses an SQLite database to store territory data and supports features such as anti-interaction, anti-block breaking, anti-block placement, anti-entity explosion, preventing general damage to entities within the territory from outsiders, territory teleportation, member management, and adding territory administrators.You can use it to protect buildings of players on endstone server.

The Territory plugin supports sub-territories, which can be created within a parent territory by the parent territory owner and administrators. Sub-territory permissions and members are independent from the parent territory. Sub-territories are fully controlled by their owners and are not controlled by the parent territory. If a parent territory is deleted, its sub-territories will lose their parent territory tag and become independent territories, and will not be deleted along with the parent territory.

The Territory plugin supports the integration of the umoney plugin as an economic system, with the economy being disabled by default. After enabling the economy in the configuration file, creating a territory will charge players based on the territory's area multiplied by the unit price. Deleting a territory will refund the player at the current price.

## Features

Territory plugin is developed using C++ and utilizes SQLite for storing territory data, theoretically offering better performance.

## How to use

> Install&Config

**Install Endstone**

Please refer to the endstone documentation for this step.

**Download & Install Territory Plugin**

Go to the Releases page to download the latest version of the plugin archive, then extract it into the plugins folder in your server directory. The directory structure is as follows:

Windows:
- plugins/
    - endstone_territory.dll # Plugin file for Windows
    - territory/ # Data directory
        - lang.json # Language file

Linux:
- plugins/
    - endstone_territory.so # Plugin file for Linux
    - territory/ # Data directory
        - lang.json # Language file

**Language**

The plugin defaults to Chinese. You can change the language by replacing the `lang.json` file inside the plugin’s data directory (`territory/`). The GitHub Releases page provides pre-packaged archives with different language versions — currently, Chinese and English are supported. You may also manually edit `lang.json` to adapt it to your preferred language.

**Config**

After running the plugin for the first time, a "territory" folder will be automatically created in the plugins directory. This folder contains the configuration file "config.json" and the territory database file "territory_data.db".

The default configuration of the configuration file is as follows:

```bash
{
    "actor_fire_attack_protect": true,
    "max_tty_area": 4000000,
    "money_with_umoney": false,
    "player_max_tty_num": 20,
    "price": 1
}
```

`actor_fire_attack_protect` indicates whether to enable creature fire protection, which is enabled by default. While the plugin can intercept direct attacks from players on creatures, if a player's weapon has a fire aspect enchantment, the enchantment effect will still apply to the creatures, causing damage and resulting in incomplete protection. After enabling creature fire protection in the configuration file, unauthorized players will be unable to inflict any damage, including fire aspect damage, on creatures within the territory. However, creatures will also gain immunity to some fire damage.

`max_tty_area` represents the maximum area for a player-created territory, with a default of 4,000,000 units (2000x2000). Territories exceeding this size cannot be created.

`money_with_umoney` indicates whether to use the umoney plugin as the economy system, which is disabled by default. When enabled, creating territories will incur charges to the player through the economic system provided by the umoney plugin.

`player_max_tty_num` is the maximum number of territories a player can own, with a default value of 20.

`price` is the price per unit area of territory, with a default value of 1. When the economy is enabled, this value serves as the price per unit area of territory for calculating the total territory cost.

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

Create New Sub_Territory

```shell
/tty add_sub Sub-Territory-corner-coordinate-1 Sub-Territory-corner-coordinate-2>
```

List Territory

```shell
/tty list
```

Delete Territory

```shell
/tty del territory-name
```

Rename Terriory

```shell
/tty rename old-name new-name
```

Set Territory Permissions

```shell
/tty set permission(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage) true|false territory-name
```

**Where the permission names respectively represent:**

- Whether to allow outsiders to interact within the territory.
- Whether to allow outsiders to destroy within the territory.
- Whether to allow outsiders to teleport to the territory.
- Whether to allow outsiders to place within the territory.
- Whether to allow entity explosions within the territory.
- Whether to allow outsiders to attack entities.

Set Territory Administrators

```shell
/tty manager add|remove player-name territory-name
```

Set Terrtory Members

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
/optty set permission(if_jiaohu|if_break|if_tp|if_build|if_bomb|if_damage) true|false territory-name
```

Reload Territory Data and Configuration

```bash
/optty reload
```
