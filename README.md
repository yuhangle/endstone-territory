# Territory Plugin

> The plugin is still under development, feedback is welcome.

[简体中文](README_zh-CN.md)  

## Introduction

The Territory plugin is a 3D territory management plugin developed in C++ for the endstone plugin loader.  It uses an SQLite database to store territory data and supports features such as anti-interaction, anti-block breaking, anti-block placement, anti-entity explosion, preventing general damage to entities within the territory from outsiders, territory teleportation, member management, and adding territory administrators.You can use it to protect buildings of players on endstone server.

The Territory plugin supports sub-territories, which can be created within a parent territory by the parent territory owner and administrators. Sub-territory permissions and members are independent from the parent territory. Sub-territories are fully controlled by their owners and are not controlled by the parent territory. If a parent territory is deleted, its sub-territories will lose their parent territory tag and become independent territories, and will not be deleted along with the parent territory.

## Features

The core of the Territory plugin has been developed in C++ since version 0.1.0, resulting in better performance and faster response times. It uses SQLite to store territory data, requiring no additional configuration and making it ready to use out of the box.

## How to use

> Install&Config

The plugin is divided into two parts: a graphical menu plugin and the core plugin. The graphical menu plugin is written in Python and does not possess the core functionality. The core plugin can run independently without the graphical menu plugin.

**Install Endstone**

Please refer to the endstone documentation for this step.

**Download & Install Territory Plugin**

> Windows

Go to Releases to download the latest version of the core plugin DLL file and the territory menu plugin WHL file, and then place them in the plugins folder of the server directory.

> Linux

Go to Releases to download the latest version of the core plugin SO file and the territory menu plugin WHL file, and then place them in the plugins folder of the server directory.

**Config**

After running the plugin for the first time, a "territory" folder will be automatically created in the plugins directory. This folder contains the configuration file "config.json" and the territory database file "territory_data.db".

The default configuration of the configuration file is as follows:

```bash
{
    "player_max_tty_num": 20,
    "actor_fire_attack_protect": true
}
```

player_max_tty_num represents the maximum number of territories a player can own, with a default value of 20.

actor_fire_attack_protect determines whether to enable entity fire protection, which is enabled by default. While the plugin can intercept direct player attacks on entities, the fire aspect enchantment on a player's weapon will still affect entities, causing damage and resulting in incomplete protection. Enabling entity fire protection in the configuration file prevents unauthorized players from dealing any damage to entities within the territory, including fire aspect damage. However, this also makes entities immune to some fire damage.

> Command Usage and Territory Management

**Command list**

Open territory menu

```shell
/ttygui
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
