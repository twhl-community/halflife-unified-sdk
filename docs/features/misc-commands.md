# Miscellaneous Commands

The Half-Life Unified SDK adds commands and variables that can be used in the console.

Commands and cvars can be client or server side. Client commands are commands that clients can execute on the server. Not all commands are autocompleted by the console.

This list of commands and variables covers anything not part of a game system. See the documentation for game systems to view their related commands.

Table of contents:
* [Server-side commands](#server-side-commands)
* [Server-side variables](#server-side-variables)
* [Client commands](#client-commands)
* [Client-side commands](#client-side-commands)
* [Client-side variables](#client-side-variables)
* [See Also](#see-also-2)

## Server-side commands

### sv_load_all_maps

Syntax: `sv_load_all_maps [map_name]`

Loads all maps and generates the node graph for them. This allows leaving the game running on its own and can also be used to generate a log containing any errors logged by the game.

The list of maps is sorted alphabetically to ensure consistent order.

The optional `map_name` parameter can be used to skip to a specific map in the list.

> <span style="background-color:darkseagreen; color: black">Note
> </br>
> Because of an [engine bug](https://github.com/ValveSoftware/halflife/issues/3409) the game will crash if too many unique models are loaded across all maps and certain other conditions are met. You will need to restart the game and continue the process by using the optional map name parameter to load all maps.</span>

### sv_stop_loading_all_maps

If the `sv_load_all_maps` was used to start automatically loading all maps, this command stops that process.

## Server-side variables

### sv_allowbunnyhopping

Controls whether players can bunny hop. Half-Life normally limits movement speed to prevent this. This cvar allows that limitation to be disabled.

### sv_bottomless_magazines

Controls whether players have bottomless magazines. If set to **0** the skill variable setting is used. If set to **1** or changed at runtime the cvar will override the skill variable setting.

### sv_infinite_ammo

Controls whether players have infinite ammo. If set to **0** the skill variable setting is used. If set to **1** or changed at runtime the cvar will override the skill variable setting.

### sv_schedule_debug

Syntax: `sv_schedule_debug <0|1>`

Controls whether schedule debugging is enabled. When enabled, if a schedule fails sparks will appear above an NPC and a `debug` message is printed on the `ent.ai` logger.

This cvar is enabled by default in debug builds to match the original behavior of this feature.

## Client commands

> <span style="background-color:darkseagreen; color: black">Note
> </br>
> Cheat commands are only allowed when **sv_cheats** is enabled.</span>

### cheat_god

> <span style="background-color:orange; color: black">This is a cheat.</span>

Player takes no health damage.

### cheat_unkillable

> <span style="background-color:orange; color: black">This is a cheat.</span>

Player health cannot drop below 1 (equivalent to Source's `buddha`).

### cheat_notarget

> <span style="background-color:orange; color: black">This is a cheat.</span>

NPCs cannot target the player (but will still hear them and turn to face them).

### cheat_noclip

> <span style="background-color:orange; color: black">This is a cheat.</span>

Fly around and through everything.

### cheat_infiniteair

> <span style="background-color:orange; color: black">This is a cheat.</span>

Player can breathe underwater.

### cheat_infinitearmor

> <span style="background-color:orange; color: black">This is a cheat.</span>

Player takes no armor damage.

### cheat_jetpack

> <span style="background-color:orange; color: black">This is a cheat.</span>

Player can fly by holding down jump button.

### cheat_givemagazine

> <span style="background-color:orange; color: black">This is a cheat.</span>

Syntax: `cheat_givemagazine [ammo_index]`

Give one magazine worth of ammo for current weapon.

The optional `ammo_index` argument can be used to specify which of the weapon's ammo types to give ammo for. Index **0** is the first ammo type.

### ent_create

Syntax: `ent_create <classname> [<key> <value> [<key2> <value2>] ...]`

Example: `ent_create weapon_9mmar respawn_delay 25` (Spawns an MP5 that respawns every 25 seconds)

> <span style="background-color:orange; color: black">This is a cheat.</span>

Creates an entity at the position you're looking at, passing along additional keyvalues if provided.

The entity's resources must have been precached before for this to work.

### ent_find_by_classname

Syntax: `ent_find_by_classname <classname> [page]`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Searches for all of the entities that have a classname matching `classname` and prints a list of matching entities.

The optional `page` argument can be used to specify which page to show.

### ent_find_by_targetname

Syntax: `ent_find_by_targetname <targetname> [page]`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Searches for all of the entities that have a targetname matching `targetname` and prints a list of matching entities.

The optional `page` argument can be used to specify which page to show.

### ent_fire

Syntax: `ent_fire <targetname> [use_type] [value]`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Triggers all entities matching `targetname`.

The optional `use_type` argument can be used to specify the use type. The default is `USE_TOGGLE`. The type must be specified using its integer value, listed below:
| Name | Value |
| --- | --- |
| USE_OFF | 0 |
| USE_ON | 1 |
| USE_SET | 2 |
| USE_TOGGLE | 3 |

The optional `value` argument can be used with entities that accept a value as input. Entities like `momentary_door` use `USE_SET` with a value to precisely control behavior.

### ent_list

Syntax: `ent_list [page]`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Prints a list of all entities.

The optional `page` argument can be used to specify which page to show.

### ent_remove

> <span style="background-color:orange; color: black">This is a cheat.</span>

Remove the first entity matching the given targetname or classname.

### ent_remove_all

> <span style="background-color:orange; color: black">This is a cheat.</span>

Remove all entities matching the given targetname or classname.

### ent_setname

> <span style="background-color:orange; color: black">This is a cheat.</span>

Sets the name of the entity you're looking at or the entity with the given targetname or classname to the given targetname.

### ent_show_bbox

Syntax: `ent_show_bbox <classname_or_targetname>`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Shows the bounding box of the first entity matching the given classname or targetname by drawing blue particle lines.

> <span style="background-color:darkseagreen; color: black">Note
> </br>
> the game expands the bounding box by 1 unit in all directions, so the box appears to be larger than the entity.</span>

### ent_show_center

Syntax: `ent_show_center <classname_or_targetname>`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Shows the center as defined by the first entity matching the given classname or targetname by creating a temporary laser dot sprite at its location.

### ent_show_origin

Syntax: `ent_show_origin <classname_or_targetname>`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Shows the origin of the first entity matching the given classname or targetname by creating a temporary laser dot sprite at its location.

> <span style="background-color:darkseagreen; color: black">
> Note
></br>
> Brush entity origins may be **0 0 0** if they do not have an origin brush.</span>

### npc_create

Syntax: `npc_create <classname> [<key> <value> [<key2> <value2>] ...]`

Example: `npc_create monster_osprey target corner1` (Spawns an Osprey that starts at the `path_corner` named `corner1`)

> <span style="background-color:orange; color: black">This is a cheat.</span>

Creates an entity at the position you're looking for and adjusts its height to place its bounding box on the ground. If the entity is stuck in an object or can't fall to the ground it is removed.

The entity's resources must have been precached before for this to work.

### set_crosshair_color

Syntax: `set_crosshair_color R G B`

Example: `set_crosshair_color 255 0 0` (bright red color)

> <span style="background-color:orange; color: black">This is a cheat.</span>

Sets the player's crosshair color to the given color.

### set_hud_color

Syntax: `set_hud_color R G B`

Example: `set_hud_color 255 0 0` (bright red color)

> <span style="background-color:orange; color: black">This is a cheat.</span>

Sets the player's HUD color to the given color.

#### See Also

* [HudColor section](game-configuration-system.md#hudcolor)

### set_suit_light_type

Syntax: `set_suit_light_type <type>`

Example: `set_suit_light_type nightvision`

> <span style="background-color:orange; color: black">This is a cheat.</span>

Sets the player's suit light to the given type.

#### See Also

* [SuitLightType section](game-configuration-system.md#suitlighttype)

### vmodenable

Same as the Half-Life command `VModEnable`, changed to follow the convention of using lowercase letters. Also exists in singleplayer to silence bogus error messages in the console.

## Client-side commands

## Client-side variables

### cl_custom_message_text

Syntax: `cl_custom_message_text "<message to display>"`

Allows players to draw a custom message on-screen.

### cl_custom_message_x and cl_custom_message_y

Syntax:
```
custom_message_x <position>
custom_message_y <position>
```

Example:
```
custom_message_x 0.1
custom_message_y -1
```
(Positions the message centered vertically and aligned near the left-hand side of the window)

X and Y positions on-screen to show the custom message. These values work the same way [game_text](https://twhl.info/wiki/page/game_text)'s coordinates work.

### cl_gibcount

The number of gibs spawned when an NPC is gibbed. Some NPCs override this to spawn a specific number of gibs.

### cl_giblife

How long gibs exist before fading out.

### cl_gibvelscale

Multiplier for gib velocity. Negative values invert the direction of the gibs.

### crosshair_scale

Crosshairs are scaled up by this amount.

### cl_rollangle

Default value: **2**

The maximum angle that the player's view rolls to when moving laterally.

### cl_rollspeed

Default value: **200**

Velocity at which roll angle is at maximum.

### cl_bobtilt

Syntax: `cl_bobtilt <0|1>`

Default value: **0**

Controls whether weapon bobbing also has tilt effects applied to it.

## See Also

* [Console Command System](console-command-system.md)
* [Client Command Registry](client-command-registry.md)
