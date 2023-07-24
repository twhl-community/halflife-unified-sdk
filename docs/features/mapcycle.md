# Mapcycle

The Half-Life Unified SDK replaces `mapcycle.txt` with a JSON-based replacement.

The file to load is specified by the cvar `mapcyclejsonfile` with a default value pointing to `cfg/server/mapcycle.json`.

The mapcycle uses the logger named `mapcycle`.

## Syntax

The mapcycle is an array whose entries are either strings containing the map name or an object containing the map name and rules to conditionally select the map.

Selection rules are the same as in `mapcycle.txt`: a map can be set to require a minimum and/or maximum number of players to be eligible. The current number of players in the server is used to evaluate these rules.

Unlike `mapcycle.txt` it is not possible to specify commands to be executed. This functionality is provided by [configuration files](game-configuration-system.md) instead.

### Object Properties

| Name | Type | Purpose |
| --- | --- | --- |
| Name | string | Name of the map |
| MinPlayers | integer | Minimum number of players required to select this as the next map |
| MaxPlayers | integer | Maximum number of players allowed to select this as the next map |

### Example

```jsonc
[
	"undertow",
	{
		"Name": "snark_pit",
		// Only switch to this map if there are between 4 and 16 players on the server
		"MinPlayers": 4,
		"MaxPlayers": 16
	},
	{
		"Name": "boot_camp",
		// Only switch to this map if there are at least 8 players on the server
		"MinPlayers": 8
	},
 	"lambda_bunker",
 	"datacore",
 	"stalkyard"
]
```
