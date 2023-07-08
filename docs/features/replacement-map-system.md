# Replacement Map System

The Half-Life Unified SDK allows you to replace certain types of assets used by the game. These replacements are looked up in a map containing identifier mappings.

Replacements can be global (affects all entities) or per-entity.

The replacement map system uses the logger named `replacementmap`.

## Configuration file syntax

Replacement map files contain a single JSON object consisting of key-value pairs. The keys are the original identifiers, the values are the replacement identifiers.

### Model replacement

This file provides replacements for models used by entities on the server side.

The filenames are case insensitive and are relative paths starting in the mod directory. The name can refer to a file in the mod directory or in a game whose content is being used as a base or fallback (i.e. `valve` or the game specified in [`liblist.gam`'s `fallback_dir`](https://developer.valvesoftware.com/wiki/The_liblist.gam_File_Structure) keyvalue).

* Supported asset types: `.mdl, .spr`

#### Entity keyvalue

`model_replacement_filename` specifies the file to use for a specific entity.

#### Example

This file provides replacements for weapon viewmodels to use Opposing Force versions instead:

```jsonc
{
	"models/v_9mmAR.mdl": "models/op4/v_9mmAR.mdl",
	"models/v_9mmhandgun.mdl": "models/op4/v_9mmhandgun.mdl",
	"models/v_357.mdl": "models/op4/v_357.mdl",
	"models/v_chub.mdl": "models/op4/v_chub.mdl",
	"models/v_crossbow.mdl": "models/op4/v_crossbow.mdl",
	"models/v_crowbar.mdl": "models/op4/v_crowbar.mdl",
	"models/v_desert_eagle.mdl": "models/op4/v_desert_eagle.mdl",
	"models/v_displacer.mdl": "models/op4/v_displacer.mdl",
	"models/v_egon.mdl": "models/op4/v_egon.mdl",
	"models/v_grenade.mdl": "models/op4/v_grenade.mdl",
	"models/v_knife.mdl": "models/op4/v_knife.mdl",
	"models/v_m40a1.mdl": "models/op4/v_m40a1.mdl",
	"models/v_penguin.mdl": "models/op4/v_penguin.mdl",
	"models/v_pipe_wrench.mdl": "models/op4/v_pipe_wrench.mdl",
	"models/v_rpg.mdl": "models/op4/v_rpg.mdl",
	"models/v_satchel.mdl": "models/op4/v_satchel.mdl",
	"models/v_satchel_radio.mdl": "models/op4/v_satchel_radio.mdl",
	"models/v_saw.mdl": "models/op4/v_saw.mdl",
	"models/v_shock.mdl": "models/op4/v_shock.mdl",
	"models/v_shotgun.mdl": "models/op4/v_shotgun.mdl",
	"models/v_spore_launcher.mdl": "models/op4/v_spore_launcher.mdl",
	"models/v_squeak.mdl": "models/op4/v_squeak.mdl",
	"models/v_tripmine.mdl": "models/op4/v_tripmine.mdl"
}
```

### Sentence replacement

This file provides replacements for sentences used by entities on the server side.

Keys and values can be either sentence groups or sentence names, but both the key and value must be the same type.

Names are case sensitive.

See the [twhl.info wiki](https://twhl.info/wiki/page/Sentences.txt) for more information about `sentences.txt`.

#### Entity keyvalue

`sentence_replacement_filename` specifies the file to use for a specific entity.

#### Example

This file provides replacements for sentences to use Opposing Force versions instead:

```jsonc
{
	// Human Grunt dialogue. Not used in Op4 since it uses different NPCs with unfiltered dialogue, but custom maps could use this.
	"HG_GREN": "OFHG_GREN",
	"HG_ALERT": "OFHG_ALERT",
	"HG_MONST": "OFHG_MONST",
	"HG_COVER": "OFHG_COVER",
	"HG_THROW": "OFHG_THROW",
	"HG_TAUNT": "OFHG_TAUNT",
	"HG_CHARGE": "OFHG_CHARGE",
	"HG_IDLE": "OFHG_IDLE",
	"HG_QUEST": "OFHG_QUEST",
	"HG_ANSWER": "OFHG_ANSWER",
	"HG_CLEAR": "OFHG_CLEAR",
	"HG_CHECK": "OFHG_CHECK",
	
	// Base computer dialogue. Used in of1a1, can probably just modify the map to use a custom sentence group.
	"C1A0_": "OFC1A0_",
	
	// HEV suit. Replaced with PCV sounds. Most of these use the same replacement so they could be merged to cut down on the number of sentences and groups.
	"HEV_AAx": "PCV_AAx",
	"HEV_A": "PCV_A",
	// Replace these separately because they are played directly.
	"HEV_A0": "PCV_A0",
	"HEV_A1": "PCV_A1"
}
```

### Sound replacement

This file provides replacements for sounds used by entities on the server side and player movement code on the client side.

The filenames are case insensitive and are relative paths starting in the mod directory. The name can refer to a file in the mod directory or in a game whose content is being used as a base or fallback (i.e. `valve` or the game specified in [`liblist.gam`'s `fallback_dir`](https://developer.valvesoftware.com/wiki/The_liblist.gam_File_Structure) keyvalue).

* Supported asset types: `.wav`

#### Entity keyvalue

`sound_replacement_filename` specifies the file to use for a specific entity.

#### Example

This file provides replacements for player footsteps to use Opposing Force versions instead:

```jsonc
{
	"player/pl_organic1.wav": "player/op4/pl_organic1.wav",
	"player/pl_organic2.wav": "player/op4/pl_organic2.wav",
	"player/pl_organic3.wav": "player/op4/pl_organic3.wav",
	"player/pl_organic4.wav": "player/op4/pl_organic4.wav",
	
	"player/pl_snow1.wav": "player/op4/pl_snow1.wav",
	"player/pl_snow2.wav": "player/op4/pl_snow2.wav",
	"player/pl_snow3.wav": "player/op4/pl_snow3.wav",
	"player/pl_snow4.wav": "player/op4/pl_snow4.wav",
	
	"player/pl_step1.wav": "player/op4/pl_step1.wav",
	"player/pl_step2.wav": "player/op4/pl_step2.wav",
	"player/pl_step3.wav": "player/op4/pl_step3.wav",
	"player/pl_step4.wav": "player/op4/pl_step4.wav",
	
	"player/pl_tile1.wav": "player/op4/pl_tile1.wav",
	"player/pl_tile2.wav": "player/op4/pl_tile2.wav",
	"player/pl_tile3.wav": "player/op4/pl_tile3.wav",
	"player/pl_tile4.wav": "player/op4/pl_tile4.wav",
	"player/pl_tile5.wav": "player/op4/pl_tile5.wav",
	
	"player/pl_wood1.wav": "player/op4/pl_wood1.wav",
	"player/pl_wood2.wav": "player/op4/pl_wood2.wav",
	"player/pl_wood3.wav": "player/op4/pl_wood3.wav",
	"player/pl_wood4.wav": "player/op4/pl_wood4.wav"
}
```

## See Also

* [Global Model Replacement](game-configuration-system.md#globalmodelreplacement)
* [Global Sentence Replacement](game-configuration-system.md#globalsentencereplacement)
* [Global Sound Replacement](game-configuration-system.md#globalsoundreplacement)
