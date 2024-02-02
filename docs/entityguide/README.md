# Unified SDK entity guide

This page lists new, modified and removed entities.

Consult TWHL's entity guide for more complete information about existing entities: https://twhl.info/wiki/page/category%3AEntity_Guides

* [Keyvalues shared between entities](keyvalues-shared.md)

## New entities

* [ambient_music](entities/ambient_music.md)
* [ammo_all](entities/ammo_all.md)
* [ammo_generic](entities/ammo_generic.md)
* [env_fog](entities/env_fog.md)
* [logic_campaignselect](entities/logic_campaignselect.md)
* [logic_isskill](entities/logic_isskill.md)
* [logic_random](entities/logic_random.md)
* [logic_setcvar](entities/logic_setcvar.md)
* [logic_setskill](entities/logic_setskill.md)
* [logic_setskillvar](entities/logic_setskillvar.md)
* [player_sethealth](entities/player_sethealth.md)
* [player_sethudcolor](entities/player_sethudcolor.md)
* [player_setsuitlighttype](entities/player_setsuitlighttype.md)
* [player_hassuit](entities/player_hassuit.md)
* [player_hasweapon](entities/player_hasweapon.md)
* [point_teleport](entities/point_teleport.md)
* [trigger_changekeyvalue](entities/trigger_changekeyvalue.md)
* [trigger_playerfreeze](entities/trigger_playerfreeze.md)

## Modified entities

* [func_breakable](entities/func_breakable.md)
* [func_healthcharger](entities/func_healthcharger.md)
* [func_pushable](entities/func_pushable.md)
* [func_recharge](entities/func_recharge.md)
* [monster_blkop_osprey](entities/monster_blkop_osprey.md)
* [monster_generic](entities/monster_generic.md)
* [monster_osprey](entities/monster_osprey.md)
* [monster_tentacle](entities/monster_tentacle.md)
* [monstermaker](entities/monstermaker.md)
* [multi_manager](entities/multi_manager.md)
* [player_weaponstrip](entities/player_weaponstrip.md)
* [trigger_changelevel](entities/trigger_changelevel.md)
* [trigger_relay](entities/trigger_relay.md)
* [trigger_teleport](entities/trigger_teleport.md)

### Modifications that affect multiple entities

* [Sound and sentence names](modifications/sound-and-sentence-names.md)

## Removed entities

* `target_cdaudio` (replaced by `ambient_music`)
* `trigger_cdaudio` (replaced by `ambient_music`)
* `func_tank_of`, `func_tankmortar_of`, `func_tankrocket_of`, `fuck_tanklaser_of`: merged into their original Half-Life variants
* `test_effect`: Never used and non-functional
* `world_items` (obsolete, replaced by individual entities)
* `item_security`
* `trip_beam` (debug only, never used)
* `cycler_weapon` (not very useful, tends to cause problems when weapons code is changed)
