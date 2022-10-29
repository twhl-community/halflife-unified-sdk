# Game Configuration System

The Half-Life Unified SDK uses a new JSON-based game configuration file which replaces Half-Life's cfg-based version.

Specifically the cvars `servercfgfile`, `lservercfgfile` and `mapchangecfgfile` and the associated files `server.cfg` and `listenserver.cfg` have been made obsolete. `server.cfg` is provided with the mod installation to silence warnings in the console.

The game configuration system uses the logger named `gamecfg`.

## Configuration file syntax

Game configuration files consist of an optional array of configuration files to include and an optional array of sections containing a name, an optional condition and properties for that section.

Includes are relative paths to configuration files that should be included before the current file. This allows you to share configuration settings. You cannot include the same file multiple times.

Sections are applied in the order that they are listed. Sections with a condition are applied only if the condition evaluates true. You can have multiple instances of the same section type, but most sections will apply only the last section of its type.

You can have both a set of includes and a set of sections.

See [conditional evaluation](/docs/features/angelscript-scripting-support.md#conditional-evaluation) for more information about conditional inclusion.

## Types of files

There are 3 types of game configuration files:
* **Server configuration file**: Executed at the start of a new map, used to initialize the server to a proper state. This file is located by using the `sv_servercfgfile` cvar. Defaults to `cfg/server/server.json`. Can be disabled by changing the cvar to an empty string.
* **Map configuration file**: Executed right after the server configuration file, this is map-specific. This file is located by searching for `cfg/maps/<mapname>.json`.
* **Map change configuration file**: Executed after the server has activated (all entities spawned and activated, except for players). This file is located by using the `sv_mapchangecfgfile` cvar. Defaults to an empty string, and so will not be used by default.

## List of section types

### Echo

This section is used for diagnostics. If the `developer` cvar is larger than zero it will be printed using the `debug` log level, otherwise it will use `trace`.

Available in all files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| Message | string | Message to print in the console |

### Commands

This section is used to execute console commands and replaces the functionality of the original cfg files.

The command syntax works just like the console and cfg files:
```jsonc
{
	"Name": "Commands",
	"Commands": [
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
}

```

Map configuration files are only allowed to execute commands listed in the file `cfg/MapConfigCommandWhitelist.json`.

The whitelist file contains an array of strings.

For example:
```jsonc
[
    "sv_gravity"
]
```

If the file is not provided then map configuration files will not be able to execute any commands.

Available in all files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| Commands | array of strings | List of commands to execute |

### GlobalModelReplacement

This section is used to specify a global model replacement file.

Only one global model replacement file may be used in a map.

Available in server and map configuration files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| FileName | string | Relative path to the replacement file |

#### See Also

* [Replacement map system](replacement-map-system.md)

### GlobalSentenceReplacement

This section is used to specify a global sentence replacement file.

Only one global sentence replacement file may be used in a map.

Available in server and map configuration files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| FileName | string | Relative path to the replacement file |

#### See Also

* [Replacement map system](replacement-map-system.md)

### GlobalSoundReplacement

This section is used to specify a global sound replacement file.

Only one global sound replacement file may be used in a map.

Available in server and map configuration files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| FileName | string | Relative path to the replacement file |

#### See Also

* [Replacement map system](replacement-map-system.md)

### HudColor

This section is used to specify a custom hud color for players to use.

The format is `R G B` where each component is a number from 0 to 255.

For example:
```jsonc
{
	"Name": "HudColor",
	"Color": "0 160 0"
}
```

Some game modes may override the hud color.

Available in server and map configuration files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| Color | string | Color to use |

### SuitLightType

This section is used to specify a custom suit light type for players to use.

The type must be one of these options:
* flashlight
* nightvision

Available in server and map configuration files.

#### Properties

| Name | Type | Purpose |
| --- | --- | --- |
| Type | string | Type of suit light to use |

## Example

This configuration file sets a hud color, suit light type and global model replacement file.

```jsonc
{
	"Sections": [
		{
			"Name": "GlobalModelReplacement",
			"FileName": "cfg/Op4ModelReplacement.json"
		},
		{
			"Name": "HudColor",
			"Color": "0 160 0"
		},
		{
			"Name": "SuitLightType",
			"Type": "nightvision"
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
