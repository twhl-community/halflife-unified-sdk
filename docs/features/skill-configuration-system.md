# Skill Configuration System

The Half-Life Unified SDK uses a new JSON-based skill configuration file which replaces Half-Life's cfg-based version.

The skill system uses the logger named `skill`.

## Configuration file syntax

`skill.json` consists of an array of sections containing an optional description, optional condition, and a set of variables.

Sections are applied in the order that they are listed. Sections with a condition are applied only if the condition evaluates true.

See [conditional evaluation](angelscript-scripting-support.md#conditional-evaluation) for more information about conditional inclusion.

Variables are a set of key-value pairs.

Keys must contain only alphabetical (upper and lowercase) and underscore characters. Numbers are allowed, but not at the start and end of the name.

The value must be a number.

You can set the values for all skill levels by providing the variable name, and you can set the value for individual skill levels by appending the skill level index to the variable name. Using both methods at the same time is not supported and will produce incorrect results.

Skill levels are:
| Level | Index |
| --- | --- |
| Easy | 1 |
| Medium | 2 |
| Hard | 3 |

### Example

```jsonc
[
	{
		"Description": "Default skill values",
		"Variables": {
			"plr_crowbar": 10,
			// HEALTH/SUIT CHARGE DISTRIBUTION
			"suitcharger1": 75,
			"suitcharger2": 50,
			"suitcharger3": 35
		}
	},
	{
		"Description": "Multiplayer-only skill values",
		"Condition": "Multiplayer",
		"Variables": {
			"suitcharger": 30,
			
			"plr_crowbar": 25
		}
	}
]
```

This configuration contains a section that defines the default values for all variables. It has no condition and so will always be applied. The second section is applied only for multiplayer games and overrides the values provided by the first section.

## Using skill variables in code

To use a skill variable in code, simply get the value like this:
```cpp
GetSkillFloat("my_variable"sv)
```

`GetSkillFloat` is a static `CBaseEntity` method that calls `g_Skill.GetValue`. It takes a `std::string_view`, so to avoid performing unnecessary string length calculations each time the [`sv` literal](https://en.cppreference.com/w/cpp/string/basic_string_view/operator%22%22sv) is used.

There is no other work required to use skill variables.

If the variable has not been defined in the configuration file the default value of `0` will be returned.

## Console commands

### sk_find

Syntax: `sk_find <search_term>`

Finds a previously defined skill variable. Partial case-sensitive matching is used to print a list of candidates.
To get a list of all defined variables use `sk_find *`.

The value of each variable will also be printed.

### sk_set

Syntax: `sk_set <name> <value>`

Sets a skill variable to the given value. If the variable does not exist it will be created.

### sk_remove

Syntax: `sk_remove <name>`

Removes the skill variable with the given name.

### sk_reset

Syntax: `sk_reset`

Removes all skill variables.

### sk_reload

Syntax: `sk_reload`

Reloads the configuration file. Changes will take effect immediately except in those cases where values have been cached in other C++ variables (e.g. an NPC's health will not change).

## Skill2Json

The `Skill2Json` tool converts original Half-Life `skill.cfg` files to the Unified SDK `skill.json` format. You can find this tool in the mod installation's `tools` directory.

Usage: `dotnet path/to/HalfLife.UnifiedSdk.Json.dll <filename> [--output-filename <output-filename>]`

## Comparison: old versus new

### The old system

In Half-Life to add a new skill variable you needed to do the following:

1. Add 3 lines to `skill.cfg`:
```
"sk_my_variable1" "10"
"sk_my_variable2" "10"
"sk_my_variable3" "10"
```

2. Add 3 cvar definitions to `game.cpp`:
```cpp
cvar_t sk_my_variable1 = {"sk_my_variable1", "0"};
cvar_t sk_my_variable2 = {"sk_my_variable2", "0"};
cvar_t sk_my_variable3 = {"sk_my_variable3", "0"};
```

3. Register all 3 cvars in `GameDLLInit`:
```cpp
CVAR_REGISTER(&sk_my_variable1); // {"sk_my_variable1","0"};
CVAR_REGISTER(&sk_my_variable2); // {"sk_my_variable2","0"};
CVAR_REGISTER(&sk_my_variable3); // {"sk_my_variable3","0"};
```

4. Add a variable to `skilldata_t`:
```cpp
float myVariable;
```

5. Synchronize the variable with the cvar in `RefreshSkillData`:
```cpp
gSkillData.myVariable = GetSkillCvar("sk_my_variable");
```

6. Optionally override the variable for multiplayer only in `CHalfLifeMultiplay::RefreshSkillData`

7. Use the variable in your code:
```cpp
pev->health = gSkillData.myVariable;
```

### The new system

1. Add 1 (to set all 3 at once) or 3 lines to `cfg/skill.json`:
```jsonc
"my_variable": 10
```

or

```jsonc
"my_variable1": 10,
"my_variable2": 10,
"my_variable3": 10
```

2. Use the variable in your code:
```cpp
pev->health = GetSkillFloat("my_variable"sv);
```
