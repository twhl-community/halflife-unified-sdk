# Keyvalues shared between entities

This page lists keyvalues that are shared between multiple entities. Not all of them may support them.

Table of contents:
* [Shared between all entities](#shared-between-all-entities)
* [Shared between all NPCs](#shared-between-all-npcs)
* [Shared between all items](#shared-between-all-items)
* [Shared between all ammo types](#shared-between-all-ammo-types)
* [Shared between all weapons](#shared-between-all-weapons)
* [Shared between all func_tank entities](#shared-between-all-func_tank-entities)

## Shared between all entities

### model

Custom model to use. If set, overrides the model used by the entity this is set on. Not all entities have a default model, and not all entities fully support custom models.

### health

Custom health to use. If set, overrides the health set by the skill configuration or the entity.

Negative values and **0** may cause malfunctions.

### model_replacement_filename

Path to the model replacement file. Must be a relative path starting in the mod directory. All search paths are checked.
#### See Also

* [Replacement Map System](../features/replacement-map-system.md)

### sound_replacement_filename

Path to the sound replacement file. Must be a relative path starting in the mod directory. All search paths are checked.
#### See Also

* [Replacement Map System](../features/replacement-map-system.md)

### sentence_replacement_filename

Path to the sentence replacement file. Must be a relative path starting in the mod directory. All search paths are checked.

#### See Also

* [Replacement Map System](../features/replacement-map-system.md)

### custom_hull_min

Syntax: `custom_hull_min X Y Z`

Custom minimum hull size. This overrides the size specified by models and entities.

### custom_hull_max

Syntax: `custom_hull_max X Y Z`

Custom maximum hull size. This overrides the size specified by models and entities.

### classification

Syntax: `classification <name>`

Default: Empty (uses classification defined by entity)

Overrides the entity's classification with a custom setting. This must be one of the classifications defined in the entity classifications configuration file.

#### See Also

* [is_player_ally keyvalue](#is_player_ally)
* [Entity Classifications](../features/entity-classifications.md)

### child_classification

Syntax: `child_classification <name>`

Default: Empty (uses classification defined by child entity)

Overrides the classification of entities created by this entity.

#### Special Values

| Value | Description |
| --- | --- |
| !owner | Sets the child entity's classification to that of the entity creating the child |
| !owner_or_default | If the entity creating the child has a custom classification, sets the child entity's classification to that of the entity creating the child. Otherwise, child classification will be left to its default value |

#### Supported Entities

* Osprey
* Black Ops Osprey
* Big Momma (Gonarch)

#### See Also

* [classification keyvalue](#classification)
* [Entity Classifications](../features/entity-classifications.md)

### unkillable

Syntax: `unkillable <0|1>`

Default: `0`

Makes entities unkillable. The entity will take damage but health will not drop below **1**.

This keyvalue is only supported by some NPCs.

## Shared between all NPCs

### allow_item_dropping

Syntax: `allow_item_dropping <0|1>`

Default: `1`

Controls whether this NPC drops items on death.

### is_player_ally

Syntax: `is_player_ally <0|1|2>`

Default: `0`

| Name | Value | Description |
| --- | --- | --- |
| Default | 0 | Uses the NPC classification's relationship towards the player |
| No | 1 | Always hostile to the player |
| Yes | 2 | Always friendly to the player |

Controls whether this NPC is allied to the player. This overrides any relationship the NPC has towards the player.

#### See Also

* [classification keyvalue](#classification)
* [Entity Classifications](../features/entity-classifications.md)

### allow_follow

Syntax: `allow_follow <0|1>`

Default: `1`

Controls whether NPCs friendly to the player can be +USEd to make them follow the player.

Some NPCs disable following by default; some disable it altogether.

> <span style="background-color:orange; color: black">Warning
> </br>
> NPC following functionality is unfinished. Only the barebones functionality has been added.</span>

### UseSentence

Syntax: `UseSentence <sentence_name>`

Sets the sentence to play when an NPC starts following the player. Some NPCs set a default sentence, most don't play anything by default.

### UnUseSentence

Syntax: `UnUseSentence <sentence_name>`

Sets the sentence to play when an NPC stops following the player by having the player +USE them to stop. Some NPCs set a default sentence, most don't play anything by default.

## Shared between all items

### respawn_delay

Custom respawn delay. If set, overrides the delay configured by the skill configuration.

A delay of **-1** disables respawn.

This allows items to respawn in singleplayer.

### stay_visible_during_respawn

Default: `0`

If enabled the item remains visible during the respawn time. Normally items are invisible until they respawn.

### flash_on_respawn

Default: `1`

If enabled the item will flash when it respawns.

### play_pickup_sound

Default: `1`

If enabled the item will play its pickup sound.

### fall_mode

Default: `0`

| Name | Value | Description |
| --- | --- | --- |
| Fall | 0 | Item falls to ground when it spawns |
| Float | 1 | Item floats in the air where it spawns |

Controls whether the item falls to the ground or floats when it spawns.

### trigger_on_spawn

Target to trigger when the item (re)spawns.

### trigger_on_despawn

Target to trigger when the item despawns. This happens when the player picks up the item.

## Shared between all ammo types

### ammo_amount

If set, the player will receive this much ammo instead of the default amount.

If set to **-1** the player will receive the maximum amount of ammo for this type.

Can be **0** to make a fake ammo entity.

## Shared between all weapons

### default_ammo

Amount of ammo contained in the weapon. This is the amount for the primary ammo type (i.e. the MP5's grenades are not affected).

If set, the player will receive this much ammo instead of the default amount when picking up the weapon for the first time.

If set to **-1** the player will receive the maximum amount of ammo for this type.

Can be **0** to make an empty weapon.

## Shared between all func_tank entities

### enemytype

Default: `0`

| Name | Value | Description |
| --- | --- | --- |
| Player only | 0 | Only target the player |
| All targets allied to player | 1 | Target all characters that are allied with the player |

Controls which targets an active tank will target.

### enable_target_laser

Default: `0`

Whether the targeting laser beam is enabled.

This is a beam showing where the tank is aiming at. Only shown for the player currently controlling the tank.

### target_laser_sprite

Default: `sprites/laserbeam.spr`

Sprite to use for the targeting laser beam.

### target_laser_width

Default: `1`

How wide the targeting laser beam is.

Must be in the range **[0, 255]**.

### target_laser_color

Syntax: `target_laser_color R G B`

Default: `255 0 0`

The color of the targeting laser beam.
