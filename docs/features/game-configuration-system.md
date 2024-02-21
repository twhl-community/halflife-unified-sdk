# Game Configuration System

The Half-Life Unified SDK uses a new JSON-based game configuration file which replaces Half-Life's cfg-based version.

Specifically the cvars `servercfgfile` and `lservercfgfile` and the associated files `server.cfg` and `listenserver.cfg` have been made obsolete. `server.cfg` is provided with the mod installation to silence warnings in the console.

The game configuration system uses the logger named `gamecfg`.

## Configuration file syntax

Game configuration files consist of an optional array of configuration files to include and an optional array of section groups. Each section group contains a set of sections and an optional condition.

Includes are relative paths to configuration files that should be included before the current file. This allows you to share configuration settings. You cannot include the same file multiple times.

Sections are applied in an unspecified order that can change when loading the same map. Sections with a condition are applied only if the condition evaluates true.

You can have both a list of includes and a set of section groups.

See [conditional evaluation](angelscript-scripting-support.md#conditional-evaluation) for more information about conditional inclusion.

Additionally a set of properties allow control over the game mode.

## Types of files

There are 2 types of game configuration files:
* **Server configuration file**: Executed at the start of a new map, used to initialize the server to a proper state. This file is located by using the `sv_servercfgfile` cvar. Defaults to `cfg/server/server.json`. Can be disabled by changing the cvar to an empty string.
* **Map configuration file**: Executed right after the server configuration file, this is map-specific. This file is located by searching for `cfg/maps/<mapname>.json`.

## Game Mode Properties

| Name | Type | Purpose |
| --- | --- | --- |
| GameMode | string | Name of the game mode to use |
| LockGameMode | boolean | If `true`, the server operator cannot override the game mode |

## List of section types

### Lists of filenames

Some sections contain a list of filenames to use. They all use the same syntax:

```jsonc
"FileNames": ["path/to/file.ext"],
"ResetList": false
```

`ResetList` can be used to clear the list of all previously added entries. Some files are added automatically by the game's code; if you intend to completely replace the contents of those files with your own then clearing the list prevents the game from needlessly loading and processing these files.

### Echo

This section is used for diagnostics. The message is logged to the console using the `gamecfg` logger on the `info` log level.

Available in all files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| Message | string | Message to print in the console |

### Commands

This section is used to execute console commands and replaces the functionality of the original cfg files.

The command syntax works just like the console and cfg files:
```jsonc
"Name": [
	// disable autoaim
	"sv_aim 0",
	
	// player bounding boxes (collisions, not clipping)
	"sv_clienttrace 3.5",

	// disable clients' ability to pause the server
	"pausable 0",

	// default server name. Change to "Bob's Server", etc.
	// "hostname \"Half-Life\"",

	// maximum client movement speed 
	"sv_maxspeed 270",

	// load ban files
	"exec listip.cfg",
	"exec banned.cfg"
]
```

Available in all files.

#### Whitelist file

Map configuration files are only allowed to execute commands listed in the file `cfg/MapConfigCommandWhitelist.json`.

The whitelist file contains an array of strings:
```jsonc
[
    "sv_gravity"
]
```

If the file is not provided then map configuration files will not be able to execute any commands.

### Sentences

This section is used to specify sentences files.

Available in map configuration files.

This section uses the [filename list syntax](#lists-of-filenames).

#### See Also

* [Sentences configuration files](sound-system.md#sentences-configuration-files)

### Materials

This section is used to specify materials files.

Available in map configuration files.

This section uses the [filename list syntax](#lists-of-filenames).

#### See Also

* [Material System](material-system.md)

### Skill

This section is used to specify skill files.

Available in map configuration files.

This section uses the [filename list syntax](#lists-of-filenames).

#### See Also

* [Skill Configuration System](skill-configuration-system.md)

### GlobalModelReplacement

This section is used to specify global model replacement files.

Available in map configuration files.

This section uses the [filename list syntax](#lists-of-filenames).

#### See Also

* [Replacement map system](replacement-map-system.md)

### GlobalSentenceReplacement

This section is used to specify global sentence replacement files.

Available in map configuration files.

This section uses the [filename list syntax](#lists-of-filenames).

#### See Also

* [Replacement map system](replacement-map-system.md)

### GlobalSoundReplacement

This section is used to specify global sound replacement files.

Available in map configuration files.

This section uses the [filename list syntax](#lists-of-filenames).

#### See Also

* [Replacement map system](replacement-map-system.md)

### HudReplacement

This section is used to specify replacements for HUD configuration files.

The section is an object optionally containing the hud replacement file and an object mapping weapon classnames to replacement filenames.

Available in map configuration files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| HudReplacementFile | string | Name of the file that replaces `hud.json` |
| Weapons | object | Map of weapon classnames to replacement filenames |

#### Example

```jsonc
"HudReplacement": {
	"HudReplacementFile": "sprites/op4/hud.json",
	"Weapons": {
		"weapon_9mmar": "sprites/op4/weapon_9mmar.json"
	}
}
```

#### See Also

* [Hud sprite system](hud-sprite-system.md)

### HudColor

This section is used to specify a custom hud color for players to use.

The format is `R G B` where each component is a number from **0** to **255**.

Some game modes may override the hud color.

Available in map configuration files.

#### Example

```jsonc
"HudColor": "0 160 0"
```

### SuitLightType

This section is used to specify a custom suit light type for players to use.

The type must be one of these options:
* **flashlight**
* **nightvision**

Available in map configuration files.

#### Example

```jsonc
"SuitLightType": "nightvision"
```

### EntityTemplates

This section is used to specify entity templates to use for specific entities.

The section is an object mapping entity classnames to filenames.

Available in map configuration files.

#### Example

```jsonc
"EntityTemplates": {
	"monster_zombie": "cfg/entitytemplates/mymaps/zombie.json"
}
```

### EntityClassifications

This section is used to specify the name of a file containing an entity classifications definition.

Only one file can be used by a map.

Available in map configuration files.

#### Example

```jsonc
"EntityClassifications": "cfg/mymaps/entity_classifications.json"
```

#### See Also

* [Entity Classifications System](entity-classifications-system.md)

### SpawnInventory

This section is used to specify the initial inventory loadout that players spawn with when they join the server and respawn after death.

The section is an object containing optional parameters.

Available in map configuration files.

#### Properties

| Name | Type | Default | Purpose |
| --- | --- | --- | --- |
| Reset | boolean | `false` | Resets Spawn Inventory configuration to its default empty state |
| HasSuit | boolean | `false` | Whether the player has the HEV Suit |
| HasLongJump | boolean | `false` | Whether the player has the Long Jump Module |
| Health | integer | `100` | Player health. Must be in the range 1-100 |
| Armor | integer | `0` | Player armor. Must be in the range 0-100 |
| Weapons | object | `{}` | Set of weapons to give to the player |
| Ammo | object | `{}` | Set of ammo to give to the player |
| WeaponToSelect | string | `""` | Which weapon to select |

`Weapons` is a set of weapon classnames mapping to an object. This object can optionally contain a `DefaultAmmo` value to specify the amount of ammo in the weapon.

`Ammo` is a set of ammo names mapping to the amount of ammo to give.

Both `DefaultAmmo` and ammo values can be **-1** to give the maximum amount.

#### Example

```jsonc
"SpawnInventory": {
	"HasSuit": true,
	"Weapons": {
		"weapon_crowbar": {},
		"weapon_9mmhandgun": {
			"DefaultAmmo": -1 // Give a pistol with maximum ammo
		}
	},
	"Ammo": {
		"Hand Grenade": 2
	},
	"WeaponToSelect": "weapon_9mmhandgun"
}
```

## Sample configuration file

This configuration file sets a hud color, suit light type and global model replacement file.

```jsonc
{
	"SectionGroups": [
		{
			// Only use this in multiplayer
			"Condition": "Multiplayer",
			"Sections": {
				"Materials": {
					// Completely override the default materials used by these maps.
					"ResetList": true,
					"FileNames": [
						"sound/mymaps/my_default_materials.json",
						"sound/mymaps/my_map_specific_materials.json"
					]
				},
				"GlobalModelReplacement": {
					"FileNames": ["cfg/Op4ModelReplacement.json"]
				},
				"HudColor": "0 160 0",
				"SuitLightType": "nightvision"
			}
		}
	]
}
```

This file includes the above file:
```jsonc
{
	"Includes": [
		"cfg/OpposingForceConfig.json"
	]
}
```
