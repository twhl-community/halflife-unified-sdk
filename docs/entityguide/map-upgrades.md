# Map Upgrades

This is a list of upgrades applied to maps when using the [ContentInstaller](../README.md#tools) or [MapUpgrader](../README.md#tools) to upgrade a map.

Some upgrades are applied to all maps, some only to maps made for a specific game.

Some upgrades are map-specific. Specific entity setups in custom maps may not be upgradeable automatically.

Updating custom maps is best done by making a copy of the original map source file and modifying it as needed. These upgrades can be used as a guideline to look for things that need changing.

Table of contents:
* [Applied to all maps](#applied-to-all-maps)
* [Half-Life-specific](#half-life-specific)
* [Opposing Force-specific](#opposing-force-specific)
* [Blue Shift-specific](#blue-shift-specific)

## Applied to all maps

### AdjustMaxRangeUpgrade

Adjusts the `MaxRange` keyvalue for specific maps to fix graphical issues
when geometry is further away than the original value.

### AdjustShotgunAnglesUpgrade

Opposing Force and Blue Shift's shotgun world model has a different alignment.
Since we're using the vanilla Half-Life model the angles need adjusting.
This adjusts the angles to match the original model.

### ChangeBell1SoundAndPitch

Find all buttons/bell1.wav sounds that have a pitch set to 80.
Change those to use an alternative sound and set their pitch to 100.

### ConvertAngleToAnglesUpgrade

Converts the obsolete `angle` keyvalue to `angles`.
This is normally done by the engine, but to avoid having to account for both keyvalues in other upgrades this is done here.

### ConvertBarneyModelUpgrade

Converts `monster_barney_dead` entities with custom body value to use the new `bodystate` keyvalue.

### ConvertBreakableItemUpgrade

Converts `func_breakable`'s spawn object keyvalue from an index to a classname.

### ConvertOtisModelUpgrade

Converts the `monster_otis` model and body value to the appropriate keyvalues.

### ConvertSoundIndicesToNamesUpgrade

Converts all entities that use sounds or sentences by index to use sound filenames or sentence names instead.

### ConvertWorldItemsToItemUpgrade

Converts `world_items` entities to their equivalent entity.

### ConvertWorldspawnGameTitleValueUpgrade

Converts the `gametitle` keyvalue to a string containing the game name.

### FixNonLoopingSoundsUpgrade

Fixes `ambient_generic` entities using non-looping sounds
to stop them from restarting when loading a save game.

### FixRenderColorFormatUpgrade

Fixes the use of invalid render color formats in some maps.

### PruneExcessMultiManagerKeysUpgrade

Prunes excess keyvalues specified for `multi_manager` entities.
In practice this only affects a handful of entities used in retinal scanner scripts.

### RemoveDMDelayFromChargersUpgrade

Removes the `dmdelay` keyvalue from charger entities. The original game ignores these.

### RenameEntityClassNamesUpgrade

Renames weapon and item classnames to their primary name.

### RenameMessagesUpgrade

Renames the messages used in `env_message` entities and `worldspawn` to use a game-specific prefix.

### ReworkGamePlayerEquipUpgrade

Changes all unnamed `game_player_equip` entities to be fired on player spawn.

### ReworkMusicPlaybackUpgrade

Reworks how music is played to use `ambient_music` instead.

### SetCustomHullForGenericMonstersUpgrade

Sets a custom hull size for `monster_generic` entities that use a model that was originally hard-coded to use one.

## Half-Life-specific

### C2a5FixBarrelPushTriggersUpgrade

Fixes the barrels in `c2a5` not flying as high as they're supposed to on modern systems due to high framerates.

### C2a5FixCrateGlobalNameUpgrade

Removes the `globalname` keyvalue from the `func_breakable` crates next to the dam in `c2a5`.
The globalname is left over from copy pasting the entity from the crates in the tunnel earlier in the map
and causes these crates to disappear.

### C3a2bFixWaterValvesUpgrade

Prevents players from soft-locking the game by turning both valves at the same time in
`c3a2b` (Lambda Core reactor water flow).

### C3a2FixLoadSavedUpgrade

Increases the reload delay after killing the scientist in `c3a2`
to allow the entire game over message to display.

### C4a2FixNihilanthDialogueUpgrade

Fixes Nihilanth's dialogue not playing at the start of `c4a2` (Gonarch's Lair).

### C4a3FixFlareSpritesUpgrade

Fixes the flare sprites shown during Nihilanth's death script using the wrong render mode.

## Opposing Force-specific

### AdjustBlackOpsSkinUpgrade

Adjust `monster_male_assassin` NPCs to use the correct head and skin value.

### ChangeFuncTankOfToFuncTankUpgrade

Renames the Opposing Force `func_tank` classes to their original versions.
No other changes are needed, as the original versions have been updated to include the new functionality.

### ConvertOtisBodyStateUpgrade

Converts `monster_otis` `bodystate` keyvalues to no longer include the `Random` value,
which is equivalent to `Holstered`.

### ConvertScientistItemUpgrade

Converts the Opposing Force scientist `clipboard` and `stick` heads to use the `item` body group instead.

### ConvertSuitToPCVUpgrade

Converts `item_suit`'s model to use `w_pcv.mdl` in Opposing Force maps.

### DisableFuncTankOfPersistenceUpgrade

Disables the `persistence` behavior for all Opposing Force tank entities to match the original's behavior.

### FixBlackOpsSpawnDelayUpgrade

Sets the `assassin4_spawn` `monstermaker` in `of6a1` to spawn a Black Ops assassin immediately
to make the switch from prisoner less obvious.

### MonsterTentacleSpawnFlagUpgrade

Converts the Opposing Force `monster_tentacle` "Use Lower Model" spawnflag to instead set a custom model on the entity,
and changes other uses to use the alternate model.

### Of0a0DisableItemDroppingUpgrade

Disables item dropping for a couple NPCs in the Opposing Force intro map so players can't get weapons from them if they die.

### Of1a1FixStretcherGunUpgrade

Updates the stretcher grunt's body value to make the grunt's weapon invisible.

### Of1a4bChangeLoaderSkinUpgrade

Changes the loader entity's skin in `of1a4b` to use the correct crate texture.

### Of2a2FixGruntBodyUpgrade

Fixes `monster_generic` entities that use `hgrunt_opfor.mdl` to use the correct body value.

### Of4a4BridgeUpgrade

Fixes the Pit Worm's Nest bridge possibly breaking if triggered too soon.

### Of5a2FixXenBullsquidScriptsUpgrade

Fixes the Bullsquids in `of5a2` having the wrong targetnames causing the eating scripts to fail.

### Of6a5FixGenewormArriveSoundUpgrade

Adds a missing `.wav` extension to the sound played when the Geneworm enters in `of6a5`.

### OfBoot1FixOspreyScriptUpgrade

Prevents the Osprey in `ofboot1` from switching to the `rotor` animation
and falling through the ground after loading a save game.

### RemoveGameModeSettingsUpgrade

Removes the CTF game mode settings from Opposing Force maps.

### RenameBlackOpsAnimationsUpgrade

Renames certain animations referenced by `scripted_sequence`s targeting `monster_male_assassin`
or entities using its model to use the new animation names.

### RenameIntroGruntAnimationsUpgrade

Renames the intro grunt animations.

### RenameOtisAnimationsUpgrade

Renames certain animations referenced by `scripted_sequence`s targeting `monster_otis`
or entities using its model to use the new animation names.

## Blue Shift-specific

### BaCanal1FixAlienSlaveSpawnsUpgrade

Removes the `netname` keyvalue from the `monstermaker`s that spawn in Alien Slaves
to prevent them from waiting for 5 seconds.

### BaOutroDisableTriggerAutoUpgrade

Sets the `Remove On Fire` spawnflag on the `trigger_auto` entity
used to start the script on `ba_outro`.
This fixes the script restarting on save load.

### BaOutroFixGruntsBodyUpgrade

Adjusts the body values of the Human Grunts in `ba_outro` to match the Half-Life script
and to avoid clipping weapons into Freeman.

### BaPower2RemoveChapterTitleUpgrade

Removes the `chaptertitle` key from `worldspawn` in `ba_power2` to remove the redundant chapter title text.

### BaSecurity2ChangeHologramModelUpgrade

Changes Gina model in `ba_security2` to allow playing push cart sequence.

### BaTram1FixScientistHeadUpgrade

Removes the `body` keyvalue from the `monster_generic` newspaper scientist in `ba_tram1`.

### BaTram1FixSuitUpgrade

Removes the HEV suit from `ba_tram1` (now given by map config).

### BaTram2FixScientistSkinColorUpgrade

Fixes the Luther scientist in `ba_tram2`'s reflective lab room having white hands.

### BaYard1FixDeadScientistModelUpgrade

Changes incorrect dead scientist head (Rosenberg) in `ba_yard1` to use the same as in `ba_yard4` (Glasses).

### BaYard4aSlavesUpgrade

Fixes the Alien Slaves in `ba_yard4a` being resurrected by triggering them
instead of the `scripted_sequence` keeping them in stasis.

### ChangeBlueShiftSentencesUpgrade

Updates references to specific sentences to use the correct vanilla Half-Life sentence.

### RemapRosenbergNoUseFlagUpgrade

Remaps `monster_rosenberg`'s `No use` flag to `monster_scientist`'s `Allow follow` keyvalue.
Technically this flag is the `Pre-Disaster` flag used by `monster_scientist`
but it is treated as a separate flag to distinguish the behavior.

### RenameConsoleCivAnimationsUpgrade

Renames certain animations referenced by `scripted_sequence`s targeting entities using `models/console_civ_scientist.mdl`.

