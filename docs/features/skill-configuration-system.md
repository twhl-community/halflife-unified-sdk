# Skill Configuration System

The Half-Life Unified SDK uses a new JSON-based skill configuration file which replaces Half-Life's cfg-based version.

The skill system uses the logger named `skill`.

## Configuration file syntax

`skill.json` consists of a set of variables. Variables are a set of key-value pairs.

Keys must contain only alphabetical (upper and lowercase) and underscore characters. Numbers are allowed, but not at the start and end of the name.

The value must be a number.

You can set the values for all skill levels by providing the variable name, and you can set the value for individual skill levels by appending the skill level index to the variable name. Using both methods at the same time is not supported and will produce incorrect results.

See the `skill.json` file included with the SDK for a list of variables.

### Skill Levels

| Level | Index |
| --- | --- |
| Easy | 1 |
| Medium | 2 |
| Hard | 3 |

### Example

```jsonc
{
	"plr_crowbar": 10,
	// HEALTH/SUIT CHARGE DISTRIBUTION
	"suitcharger1": 75,
	"suitcharger2": 50,
	"suitcharger3": 35
}
```

## Using skill variables in code

To use a skill variable in code, simply get the value like this:
```cpp
GetSkillFloat("my_variable"sv)
```

`GetSkillFloat` is a static `CBaseEntity` method that calls `g_Skill.GetValue`. It takes a `std::string_view`, so as to avoid performing unnecessary string length calculations each time the [`sv` literal](https://en.cppreference.com/w/cpp/string/basic_string_view/operator%22%22sv) is used.

There is no other work required to use skill variables.

If the variable has not been defined in the configuration file the default value of `0` will be returned.

### Constraints

Skill variables can be explicitly defined in code to add constraints to them. This is done in `ServerLibrary::DefineSkillVariables`.

Variables should only be defined on the server side. The client receives information about all networked variables on connect.

Defining a variable without constraints is done simply by doing this:
```cpp
g_Skill.DefineVariable("my_variable", 60);
```

This defines a variable `my_variable` with a default value of **60**.

Constraints are added by using the last parameter:
```cpp
g_Skill.DefineVariable("my_variable", 60, {.Minimum = -1});
```

This adds a minimum value constraint of **-1**. Values smaller than this will be clamped to the constraint.

Multiple constraints can be added like this:
```cpp
g_Skill.DefineVariable("chainsaw_melee", 0, {.Minimum = 0, .Maximum = 1, .Networked = true, .Type = SkillVarType::Integer});
```

List of constraints:
| Name | Purpose |
| --- | --- |
| Minimum | Minimum value to allow |
| Maximum | Maximum value to allow |
| Networked | If true, the value is sent to clients to allow client-side code to access it |
| Type | Type of the value. By default all skill variables are floats, this allows the value to be round to the nearest integer value by setting the type to `SkillVarType::Integer` |

## Console commands

### sk_find

Syntax: `sk_find <search_term> [filter]`

Finds a previously defined skill variable. Partial case-sensitive matching is used to print a list of candidates.
To get a list of all defined variables use `sk_find *`.

The value of each variable will also be printed.

To filter variables by type, use one of these filter names:
| Filter | Type |
| --- | --- |
| all | All variables will be checked |
| networkedonly | Only variables marked as networked will be checked |

### sk_set

Syntax: `sk_set <name> <value>`

Sets a skill variable to the given value. If the variable does not exist it will be created.

## Skill2Json

The `Skill2Json` tool converts original Half-Life `skill.cfg` files to the Unified SDK `skill.json` format. You can find this tool in the mod installation's `tools` directory.

Usage: `dotnet path/to/Skill2Json.dll <filename> [--output-filename <output-filename>]`

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
