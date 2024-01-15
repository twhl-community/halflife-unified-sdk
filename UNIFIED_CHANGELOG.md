# Half-Life Unified SDK V1.0.0 Changelog

This is the changelog from Half-Life Updated version 1.0.0 to Half-Life Unified SDK V1.0.0.

> <span style="background-color:darkseagreen; color: black">Note
> </br>
> This changelog does not include changes that were made in Half-Life Updated and related projects. See [FULL_UPDATED_CHANGELOG.md](FULL_UPDATED_CHANGELOG.md) for a list of those changes.</span>

Table of contents:
* [New Features](#new-features)
* [Entities](#entities)
* [NPCs](#npcs)
* [Items And Weapons](#items-and-weapons)
* [Audio](#audio)
* [User Interface](#user-interface)
* [Bug Fixes](#bug-fixes)
* [Code Cleanup](#code-cleanup)
* [Project Changes](#project-changes)
* [Tools](#tools)
	* [Studiomdl](#studiomdl)
* [Prototypes](#prototypes)
* [Asset Changes](#asset-changes)

## New Features

* Implemented **JSON file loading and error logging** with **optional JSON Schema-based validation** support
* **Game configuration files**: JSON-based configuration files that allow server operators and level designers to configure the game to their needs
	* Console commands can be set by map configuration files only if they are whitelisted by the server operators
* Converted **skill system** to use JSON
	* Skill variables can now be **constrained** and can be **networked to clients**
* Converted the **map cycle** to use JSON
* Implemented **network data transfer system using files** to pass data from server to client
	* Some cvars are forced on to make this work. Server operators are warned if a cvar is forced on to alert them to the change
* New **string pool** that eliminates the use of the engine's memory pool and prevents repeated memory allocations for the same string
* Access to **IFileSystem** interface
	* Reworked all file access to be routed through this interface
	* Prevent paths passed to filesystem functions from containing `..` (parent directory)
* Implemented **console command system** to allow object member functions to be console command handlers and to provide the same interface for client and server code
* **Server-side client commands** are now registrable and unregistrable on the fly to allow for gamemode-specific commands
* Implemented **client user messages dispatcher** to allow object member functions to be user message handlers
* **Configurable hud and crosshair colors**: settings are map-specific and can be overridden using entities and cheat commands on a per-player basis
* **Configurable suit light type** (**flashlight** or **night vision**): setting is map-specific and can be overridden using an entity and a cheat command on a per-player basis
* Implemented **Campaign selection menu** (replaces **New Game dialog**)
* Reworked **HUD sprite system**
	* Converted HUD sprite configuration files to use JSON
	* Implemented **HUD replacement system**: allows maps to provide **custom HUD sprite configuration files**
* Implemented **crosshair scale** option (**OpenGL renderer only**)
* Implemented new client-side temporary entity list; increased maximum number from **500** to **2048**
* Implemented **entity templates**: allows default values for keyvalues to be specified for a specific type of entity in a map (e.g. amount of ammo given by ammunition, amount of ammo in a weapon)
* Implemented **global and per-entity model, sound and sentence replacement**. **Global sound replacement also works on the client side**
	* Can be configured on a map-to-map basis
	* Per-entity sentence replacement is used to handle **Otis**, **Drill Sergeant**, **Recruit** and **Rosenberg** sentence changes, removed code duplication in these classes
* Implemented **Spawn Inventory system**: Allows level designers to specify the initial contents of a player's inventory when they (re)spawn
* Implemented **Persistent Inventory system in Co-op**: allows players to **carry over their inventory, health and armor** across level changes
* Reworked gamerules to rely more on **configurable behavior** instead of hard-coding gamemode-specific rules (e.g. item respawn rules are now configured in skill files, not in the gamemode-specific gamerules class)
* Reworked **game mode selection**: can be controlled by the server operators and overridden on a map-to-map basis
* Added **barebones bot system for multiplayer testing**
* Added `cl_custom_message_text` cvar to **display a custom message on-screen**, along with `cl_custom_message_x` and `cl_custom_message_y` to control the position on-screen (works like `game_text`)
* Added `sv_load_all_maps` command to load all maps and generate the node graph for them (allows leaving the game running on its own, can also be used to generate a log containing any errors logged by the game) along with `stop_loading_all_maps` command to stop this process
* Added cvars `sv_infinite_ammo` & `sv_bottomless_magazines` to override skill variables
* Added **cheat commands that work in all gamemodes**:
	* `cheat_god`
	* `cheat_unkillable`
	* `cheat_notarget`
	* `cheat_noclip`
	* `cheat_infiniteair`
	* `cheat_infinitearmor`
	* `cheat_jetpack`
	* `cheat_givemagazine`
	* `ent_create`
	* `ent_find_by_classname`
	* `ent_find_by_targetname`
	* `ent_fire`
	* `ent_list`
	* `ent_remove`
	* `ent_remove_all`
	* `ent_setname`
	* `ent_show_bbox`
	* `ent_show_center`
	* `ent_show_origin`
	* `npc_create`
	* `set_hud_color`
	* `set_suit_light_type`

## Entities

* Set max edicts to **2048** in `liblist.gam`, log actual max edicts on map start (max edicts is calculated as `max_edicts + ((maxplayers - 1) * 15)`)
* Implemented **strongly typed entity handles**
* Added **entity dictionary** to create entities with
* Reworked entity creation and destruction to allow additional logic to be added around the `OnCreate` and `OnDestroy` function calls
* Added support for **custom hull sizes** to all entities. This setting should be used only with entities that use **Studio models** (files with `.mdl` extension)
	* Removed obsolete workaround for custom hull size in `monster_generic`, now relies on mapper-defined hull sizes for these
* Added **custom health** keyvalue
* Added **custom model** keyvalue
* Added `game_playeractivate` target, fired when the player activates (finished joining the server/loading save game and finished initializing)
* Removed **obsolete alternate entity classnames for weapons** (e.g. `weapon_glock` was an alternate for `weapon_9mmhandgun`)
* Implemented target selector support for `target`, `killtarget` and `point_teleport` (`!activator` and `!caller`)
* Ensured all worldspawn keyvalues restore correctly on save game load. The `WaveHeight` keyvalue did not get set correctly on save game load causing water that isn't `func_water` to use the wave height that was last set
* Replaced tracktrain event with proper throttled sound updates. This reduces the amount of work that the engine has to do to handle train pitch sound updates and removes some client side code that is now back on the server side like it was in SDK 1.0
* **Restart track train sound if a player joins** (looping sounds don't play for players if they started before they joined)
* **Converted entities to use sound/sentence names instead of indices**. The following entities now allow you to specify sound filenames or sentence names (for (un)locked sentence) instead of using combo boxes:
	* `env_spritetrain`
	* `func_button`
	* `func_door`
	* `func_door_rotating`
	* `func_plat`
	* `func_platrot`
	* `func_rot_button`
	* `func_rotating`
	* `func_train`
	* `func_trackautochange`
	* `func_trackchange`
	* `func_tracktrain`
	* `func_water`
	* `momentary_door`
	* `momentary_rot_button`
* Added diagnostic output warning if there are too many `multi_manager` targets
* Added diagnostic output for when `multi_manager`s activate
* Increased maximum number of `multi_manager` targets from **16** to **64**
* Implemented **targeting laser** in all `func_tank` entities
* Re-implemented `UTIL_FindEntityBy*` functions to avoid calling engine functions
	* Implemented wildcard matching for entity `classname`, `targetname` and `target` searches
* Added master support to `trigger_relay`
* Reworked `func_healthcharger` and `func_recharge` (HEV charger) to use shared base class
* Added the option to strip the HEV suit and long jump module from players using `player_weaponstrip`
* Reworked `item_generic` to avoid the possibility of the wrong sequence being used if the game saves right between the entity spawning and finishing its initialization
* Made `trigger_changelevel` **work in multiplayer** (**non-persistent level changes only**)
	* Added the option to use **non-persistent level changes** in singleplayer
* Removed unused `trigger_changelevel` functionality left over from Quake 1
* Added check to prevent level changes from breaking if they are too close to each-other in relative space. The game will instead notify players who touch broken level changes that the level change is incorrectly placed
* Reworked `func_breakable` and `func_pushable` to allow level designers to **specify the classname of the entity to spawn on break**. This allows both entities to spawn any item in the game aside from CTF items without needing code changes to support newly added items
* Reworked save/restore system to use a datamap to store data
	* Decoupled engine save/restore data from the game's data to allow the game's version to be changed freely
	* Use serializers for each type
	* Fixed arrays of function pointers not saving and restoring correctly
	* Added function table to datamap to store the list of entity functions used as `Think/Use/Touch/Blocked` in the server library instead of relying on the engine to look them up (eliminates use of platform-specific functionality)
* Added entities:
	* `ambient_music`
	* `ammo_all`: Gives all ammo types. Configurable amount
	* `ammo_generic`: Gives ammo of a specified ammo type (combine with `trigger_changekeyvalue` to dynamically change ammo type)
	* `env_fog`: Counter-Strike style fog
	* `logic_campaignselect`: Shows the campaign selection menu
	* `logic_isskill`: fires a target matching the current skill level
	* `logic_random`: triggers a randomly selected target out of up to **16** targets
	* `logic_setcvar`
	* `logic_setskill`: Changes the skill level setting. Due to how the skill system works some changes will only take effect after a map change. This entity is the equivalent of Quake 1's `trigger_setskill` but is a point entity that needs to be triggered. Its intended purpose is to be used in a campaign selection map
	* `logic_setskillvar`: Sets a specific skill variable to a value. Intended for multiplayer only because skill variables are not saved and restored, and are reset on map load based on specified skill config files
	* `player_hassuit`
	* `player_hasweapon`
	* `player_sethealth`
	* `player_sethudcolor`
	* `player_setsuitlighttype`
	* `point_teleport`: Teleports a targeted entity to its own location
	* `trigger_changekeyvalue`
* Removed entities:
	* `trigger_cdaudio`
	* `target_cdaudio`
	* `func_tank_of`, `func_tankmortar_of`, `func_tankrocket_of`, `fuck_tanklaser_of`: merged into their original Half-Life variants
	* `test_effect`: Never used and non-functional
	* `world_items` (obsolete, replaced by individual entities)
	* `item_security`
	* `trip_beam` (debug only, never used)
	* `cycler_prdroid` (nonfunctional)
	* `cycler_weapon` (not very useful, tends to cause problems when weapons code is changed)
	* `cine_*` entities (nonfunctional)
	* `CBaseSpecator` (never used, did not function)

## NPCs

* Changed schedule debug logic (spark effect above NPC's head) to be controlled by `sv_schedule_debug` cvar
* Fire Death trigger condition when turret NPCs are destroyed
* Reworked Nihilanth and Gene Worm death logic to not teleport players in multiplayer (used to trigger the end-of-map script)
* Fire `monstermaker` target after NPC spawning is done instead of after NPC creation and before spawning
* Added a way to pass arbitrary keyvalues to `monstermaker` children
* Added `unkillable` keyvalue: Makes NPCs unkillable. Not all NPCs support this setting
* Overhauled **entity classification system**
	* Implemented `classification` keyvalue
	* Implemented `child_classification` keyvalue: allows NPCs that spawn NPCs (e.g. Big Momma, Osprey) to pass on classifications
	* Implemented `is_player_ally` keyvalue
	* Simplified talk monster friendly check and use/unuse sentence configuration. Friendly NPCs like scientists and security guards will no longer follow players if their classification treats players as anything other than allies (i.e. as if the player has shot them while not in combat)
	* Made it possible to for any NPC to follow players if they are friendly to the player (barebones support only)
    	* Disabled following for some NPCs
* Fixed `scripted_sequence` searching for targets incorrectly
* Added `initial_capacity` keyvalue to Osprey NPCs to allow specifying how many grunts/assassins to deploy instead of relying on existing NPCs in the map to influence calculations (maximum of **24**)
* Forcefully reset monster ideal activity when released from Barnacle (previously depended on task-specific behavior)
* Re-added activity changes to talk monster schedules using `ACT_IDLE` instead of `ACT_SIGNAL3` (not used by talk monsters, but is used by allied human grunts which do have that activity)
* Fixed Barney playing post-disaster use/unuse sentences in training
* Fixed Human Grunts and Male Assassins not playing MP5 attack sounds during scripted sequence
* Fixed rats having alien gibs instead of human gibs
* Fixed turrets and tanks continuing to target players after enabling notarget
* Added keyvalues to set Barney & Otis corpse bodygroups

## Items And Weapons

* Changed weapon precaching to use weapon dictionary, client side code now creates all weapons using this dictionary instead of requiring globals to be added manually
* Moved weapon class declarations out of `weapons.h` since they should not be accessed directly
* Reworked RPG state networking to eliminate use of special case. Nearly all references to `WEAPON_*` constants are contained in their individual weapons now
* Changed how weapon selection is handled to use a dedicated client command instead of relying on command and weapon classnames starting with the `weapon_` prefix
* Reworked the weapons code to use skill variables to control multiplayer behavior (allows alternate weapon behavior to be controlled in any game mode on a case-by-case basis)
* Ensured auto weapon switch setting is always initialized
* Made the Knife backstab attack work against NPCs
* Removed `sv_oldgrapple` cvar (leftover from an old Opposing Force update)
* Reworked `weapon_rpg` and `ammo_rpgclip` to use entity templates for multiplayer-only ammo doubling
* Refactored weapons, ammo and pickup items (excluding Opposing Force CTF items) to use a shared base class
	* The `give` and `impulse 101` commands now use the item dictionary preventing them from spawning other types of entities
* Made `impulse 101` give all weapons using the weapons dictionary, give the suit without playing the login sound and give max ammo without creating ammo entities
	* The MP5 now has a full magazine when given using this cheat
* Don't switch weapons when using `impulse 101` if the player has a weapon equipped already
* Merged server and client versions of weapons code when possible
* Merged `CBasePlayerItem` and `CBasePlayerWeapon` (items only work properly when they are weapons anyway, some code assumes all items are weapons and no classes inherit from `CBasePlayerItem` directly)
* Reworked ammo type definitions so the types are explicitly defined instead of implicitly through weapons referencing them. The ammo type definition now includes the maximum capacity. Code that required the capacity to be passed in now queries the ammo type system for this information instead. The list of ammo types is sent to the client as well
* Reworked weapon info to be networked to client through the new networking system, `WeaponList` network message removed
* Reworked tripmine entities to use separate world model
* Added `ITEM_FLAG_SELECTONEMPTY` flag to weapons that regenerate ammo to always be selectable and never show as red in hud history

## Audio

* OpenAL-based **music and sound systems**
	* **Music no longer stops playing after a level change**
	* Converted sentences configuration files to use JSON
	* Added support for **multiple sentences configuration files** along with the ability to **redefine sentences and sentence groups**
	* Raised maximum number of **concurrent sounds** from **128** to **unlimited**
	* Raised maximum number of **sound precaches** from **512** to **65535**
	* Raised maximum number of **sentences** from **1536** to **65535**
	* Raised maximum number of **sentence groups** from **200** to **unlimited**
	* Reworked sentence groups to **allow sentences to be non-sequential**
	* Maximum number of **sentences in a group**: was **32**, now **unlimited**
	* Raised maximum **sentence name length** from **15** ASCII characters to **unlimited**
	* Raised maximum **number of words in sentence** from **32** to **unlimited**
	* Maximum number of **unique sounds** (server + client side sounds): was **1024**, now **unlimited**
	* **Sentences are no longer cut off after pausing and resuming**
	* Implemented **EAX** effects
	* Implemented **Aureal A3D** functionality using **HRTF** (note: experimental)
	* Made sounds with attenuation 0 play at player's position (fixes `ambient_generic` **Play Everywhere** sounds still using spatialization)
	* Music and sounds can be one of the following file formats:
		* Wave
    	* MP3
    	* Ogg Vorbis
    	* Ogg Opus
    	* FLAC
    	* WavPack
    	* Musepack
* Reworked **material system**
	* Converted to JSON
	* Added support for **multiple materials configuration files**
	* Raised maximum **material texture name length** from **12** to **15** (matches WAD and BSP limits)
* Play hud and geiger sounds on unique channels to avoid preempting game sounds (geiger used to cause sounds to cut out)
* Reworked HEV suit sentence playback to no longer store group and sentence indices in save games (indices are not guaranteed to be stable)

## User Interface

* HUD no longer assumes HUD sprites have sequential indices when loaded from configuration file
* Reworked armor HUD to avoid depending on relative positions of the full and empty icons in the sprite sheet
* Reworked HUD message drawing to allow for unlimited-length lines
* Merged armor HUD into health HUD, draw armor next to health regardless of resolution (matches original game behavior at 640x480 resolution)
* Added new HUD elements:
	* Project info overlay: Shows information about the client and server libraries, Git information and other development information
	* Entity info: Shows information about the entity the player is looking at
	* Debug info: Shows information about the player, the map and other game state
* Raised maximum number of weapon categories from **7** to **10**
* Reworked the HUD weapon list to allow an unlimited number of weapons in each weapon category (previously limited to **7** weapons total in Opposing Force, **5** in Half-Life)
* Weapon categories above **5** are only drawn if there are weapons in that category or a category with a greater number (makes HUD look like Half-Life's when no Opposing Force weapons are equipped)
* Reworked train control and pain sprites to use `hud.json` entry for name mapping to enable an Opposing Force-specific version to be used
* Moved chat input position to 2 lines above the chat text area instead of at the top of the screen
* `sv_spamdelay` is now ignored in singleplayer (it controls the time before players are allowed to chat again, to limit the amount of chat text allowed at any given time)
* Reworked game title drawing to support drawing all three games' titles
* Removed special support for resolutions smaller than **640** pixels in width; the game will now use assets designed for higher resolutions when running with a low resolution
* Implemented console print buffering during client startup. This is necessary to allow regular console output to appear since the engine ignores it until the engine has finished initializing itself

## Bug Fixes

* Re-implemented spectator mode to function in all gamemodes
* Fixed `cycler_wreckage` storing time value in `int` instead of `float`
* Fixed items dropped by NPCs respawning in multiplayer
* Prevent entity classnames from being changed by level designers using entities that can change arbitrary keyvalues (e.g. `trigger_changekeyvalue`)
* Fixed `game_text` messages not resetting time value correctly causing messages that reuse channels to use the wrong start time
* Disable `trigger_teleport` when target is an empty string (prevents teleportation to seemingly random locations)
* Always store activator on delay trigger to allow access to initial activator entity in delayed trigger setups
* Stop `func_train` move sound when triggered off (fixes sound looping issue in `c1a0c` (first Unforeseen Consequences map))
* Fixed `env_blowercannon` crashing if target does not exist
* Limited `Overflow 2048 temporary ents!` message to only print once per frame (excessive logging caused framerate drops to < 30 FPS)
* Clear global state when `game_end` is used to end multiplayer game (allows Co-op maps to clear state that may affect other maps)
* Fixed Turrets and Spore ammo entities not playing sounds in multiplayer in some cases
* Fixed `momentary_rot_button` not stopping its sound
* Fixed `momentary_door` trying to play null `string_t`. This caused warnings to be printed in the console
* Fixed ropes breaking at high framerates
* Reworked NPC-spawned gibs to be created as temporary entities to eliminate problems with `ED_Alloc: out of edicts errors`, crashes and entity rendering issues
* Fixed `monstermaker` with infinite children not spawning an infinite number (will spawn **4,294,967,293** entities and then stops)
* Made sure entities notify their owner if they are killtargeted so they can keep track of how many active children there are (needed for `monstermaker` and `monster_bigmomma`)
* Fixed entities not removing child entities created by them (e.g. beam effects created by Alien Slaves)
* Fixed Alien Slave beams staying forever if they exist during a level change
* Fixed Apache and Osprey not playing rotor sounds correctly in multiplayer (they use singleplayer-only pitch modulation logic that depends on the local player entity)
* Fixed Human Grunts dropping weapons again if the game is saved and loaded while the grunt is dying
* Ensure Turret NPCs finish dying so they don't keep running dying logic forever if killed during first activation
* Added null check to scientist fear task to prevent potential crashes
* Fixed Female Assassin footsteps always playing every **0.2** seconds when walking and running (now uses the same intervals as player, but may differ somewhat because NPCs only think 10 times a second)
* Fixed Female Assassins not resetting their movetype if they jump and land while not in combat (resulted in stuttering movement because they were considered to still be in the air)
* Fixed Medic Grunt always trying to follow the local player even if another player interacted with them
* Removed invalid cast in Torch Grunt death logic
* Fixed weapons marked as "limit in world" (e.g. Hand Grenade) respawning at the wrong time if the server is near the entity limit
* Fixed player weapons still receiving input when starting to use a `func_tank`
* Fixed `func_tank` entities not returning player weapon control when killtargeted
* The weapon deploy animation is now always sent to clients when calling `EquipWeapon` to ensure it plays when the player stops using `func_tank` entities
* `func_tank` entities can now switch targets if their current target is behind cover
* Disabled the ability to pick up items when in observer/spectator mode
* Fixed Gauss gun sometimes setting remaining player Uranium ammo to **-1**
* Fixed Glock not playing empty sound when using secondary attack
* Fixed Health Charger recharge time not using the correct value in Co-op when using `mp_coopweprespawn 1`
* Sync client with server Gauss charge time value so spin sound pitch is correct after level change
* Fixed user interface coordinates and sizes being incorrectly adjusted for resolution (caused spectator interface buttons to overlap)
* Fixed VGUI1 `CListBox` class setting scrollbar range to incorrect maximum (off by one issue, last line was always empty as a result)
* Fixed spray logo using the wrong decal after a save game load when not using a custom spray (i.e. if you use the **default small Lambda spray**, not the custom one defined in the **Multiplayer options page**)
* Reset sky name to `desert` to ensure sky name is consistent even after loading other maps that have other sky names (e.g. `crossfire` does not specify a sky name and uses the last used sky)

## Code Cleanup

* Reduced number of required headers needed by source files in the server and client libraries to simplify creation of new source files
* Replaced `min` and `max` macros with `std::min`, `std::max` and `std::clamp`
* Added assert to warn about classnames with uppercase letters
* Merged variables used to store the name of an entity's master
* Moved `m_hActivator` to `CBaseEntity` and removed a hack added post-release
* Renamed all entities to use lowercase letters (`weapon_9mmAR`, `ammo_9mmAR`, `ammo_ARgrenades`)
* Renamed `TakeHealth` to `GiveHealth` because positive amounts give health
* Removed duplicated ammo variables used in prediction code
* Moved weapon-specific variables from `CBaseEntity` to their respective classes
* Store player suit flag in separate variable, removed unused `WEAPON_CHAINGUN` constant (effectively frees up 2 weapon slots for use)
* Overhauled NPC schedule list code to use a simpler solution
* Reworked Opposing Force CTF stats menu to use safe format strings and to use a fallback string if the file providing it is missing or contains a bad format strings
* Reworked client side message parsing to use a class instead of global variables (saves a little memory and eliminates edge cases from leftover parsing state and missing parse initialization)
* Marked engine functions as deprecated if they do nothing, do nothing useful or if a local function does the same (will produce compiler warning if you try to use such a function)
* Reworked code that always uses the local player to work properly in multiplayer
* Use more consistent method to check if the game is multiplayer or singleplayer and whether it's a teamplay gamemode
* Reworked how the player's reserve ammo and each weapon's magazine is accessed to centralize the logic
* Reworked casts to `CBasePlayer` to use `ToBasePlayer` function (extra safety checks and runtime type validation in debug builds)
* Reworked utility functions involving players to take `CBasePlayer*` (avoids passing non-player entities to these functions and simplifies function calls)
* Reworked code that uses `entvars_t::enemy` to use a separate variable when possible (the engine uses this variable in NPC movement code but it doesn't seem to actually be used)
* Reworked CTF item detection to avoid use of entity classification (uses `MyCTFItemPointer()` function now)
* Reworked how the game determines which kind of gibs to spawn (human or alien gibs, relied on the entity classification before which isn't always accurate)
* Reworked some string buffers and operations to avoid buffer overflows or truncations
* Reworked weapon functions that start animations to use `pev->body` instead of a separate `body` parameter
* Consolidated math functions in `mathlib.h/.cpp`
* Link user messages on server startup instead of after map spawn (fixes edge case where messages are not registered yet when used)
* Converted as much code as possible to use `Vector` instead of `float arrays/pointers`, replaced older vector math functions with `Vector` operators and helpers
* Use local `AngleVectors` functions instead of engine function (more efficient, compiler can optimize the calls since it can see the implementation)
* Identify players consistently using the `IsPlayer()` function
* Changed all uses of `pfnGetPlayerWONId` (obsolete, always returns `UINT32_MAX`) to `pfnGetPlayerAuthId` (Steam Id)
* Replaced `DECLARE_COMMAND` and `HOOK_COMMAND` macros with console command system
* Added virtual destructor to `CGameRules`

## Project Changes

* Renamed `hl` library to `server`
* Requires C++20
* CMake used to generate project files
	* CMake options added to help streamline development and testing
	* **vcpkg** used to acquire third party dependencies (except for dependencies that are included in the SDK like **SDL2**)
* Clang-tidy used to detect potential problems
* Clang-format used to enforce consistent code formatting across the codebase
* Continuous integration builds Windows and Linux versions to automatically detect problems
* Vastly improved logging support using **spdlog** loggers; existing logging code uses separate loggers for fine-grained control
	* Diagnostic logging for map startup stages (helps to pinpoint when other log output occurs)
	* Diagnostic logging to show time spent loading server config files
	* Diagnostic logging for precache calls. This allows you to see which models, sounds and generic file precaches are occurring, but does not include precache calls made by the engine
* **fmtlib** used to formats strings easily, without truncation, buffer overflows or incorrect type conversions
* Added **EASTL** library to make use of specialized containers
* Added hint file to stop Visual Studio's Intellisense from marking certain macros as function declarations with no definition
* Added natvis file to Allow Visual Studio's debugger to show the contents of certain data types used in the SDK (e.g. `string_t` string contents)
* Added option to use Link-time optimizations (C++ compiler setting)
* Reworked much of the C++ code to enable new features to be implemented and to make it more robust
	* Removed unused variables and C-style casts
	* Converted entity code to use `CBaseEntity` instead of `edict_t`, `entvars_t`, `EOFFSET` and entity indices wherever possible to normalize the use of the same data type for all entity code
	* Use entity handles to avoid potential dangling pointers
	* Removed unused `CHealthPanel` class (non-functional VGUI1 version of the health HUD)
	* Reworked UI code to no longer use resolution size specifier
* Converted much of the documentation comments to Doxygen syntax
* `string_t` is now a separate type that does not implicitly convert to and from integers
* Removed `game.cfg` execution
* Additional debugging features to detect invalid uses of entity function pointers in debug builds
* Added `SV_SaveGameComment` function: used by the engine to fill in the text shown in the save game description in the **Load Game dialog** (game uses localized text for this)
* Added safety checks to prevent server state corruption if multiple map change commands occur in the same frame
* Prevent engine precache limit from being reached (game will no longer tell engine to precache a resource if doing so would cause the limit to be exceeded)
* Fixed Linux libraries having visible symbols

<div class="page"/>

## Tools

* Tools now obtain SDL2 using `vcpkg` instead of relying on the SDL2 library included with the SDK (allows 64 bit builds to be made)
* Fixed `serverctrl` tool not compiling when built as 64 bit (note: tool is not compatible with Steam Half-Life due to removed engine functionality)
* Removed `procinfo` (**proc**essor **info**) library (obsolete, the Half-Life SDK uses SSE2 by default which is only available with CPUs newer than what this library tests for)
* Fixed `qcsg` and `qrad` crashing due to pointer-to-int downconversion in 64 bit builds
* Removed duplicate `winding_t`, `CopyWinding`, `FreeWinding`, `WindingCenter` and `pw` functions
* Resolved conflicts between `wadlib` and `textures.cpp`
* renamed `nummodels` in `studiomdl` to `numstudiomodels` to avoid conflict with `bspfile.h`

### Studiomdl

* Mark bones that are referenced by attachment as used. This allows bones that have no vertices attached to it to remain in the model if an attachment references it. `v_shock.mdl` requires this to fix a visual glitch experienced by some players where polygons meant to be invisible were being rendered due to an OpenGL glitch

<div class="page"/>

## Prototypes

* [GameNetworkingSockets networking system](docs/prototypes/nonfunctional-gns-networking-system.md)
* [Angelscript scripting system](docs/prototypes/nonfunctional-scripting-system.md)

## Asset Changes

*Main changelog*: [Assets Changelog](https://github.com/twhl-community/halflife-unified-sdk-assets/blob/master/CHANGELOG.md)

* Many models have been updated to include all data in a single file (as opposed to split into a main and texture file along with sequence group files)
* Variants of the same model from different games and low and high definitions versions have been standardized to include the same properties
* Various animation issues have been fixed
* HUD sprites have been reworked to reduce the number of sprites used
* Variants of the Opposing Force HUD without scanline effects have been added
