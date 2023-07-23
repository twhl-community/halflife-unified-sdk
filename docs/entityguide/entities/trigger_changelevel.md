# trigger_changelevel

**Brush entity**

Brush entity that allows players to move to other levels.

> <span style="background-color:darkseagreen; color: black">
> Note
></br>
> Only changes are listed in this documentation. Consult TWHL's page for more information.</span>

[TWHL:trigger_changelevel](https://twhl.info/wiki/page/trigger_changelevel)

## Keyvalues

* **use_persistent_level_change** - If enabled players will carry over their inventory in singleplayer and landmarks are used to carry over other entities. Default `1`
* **use_persistent_inventory** - If enabled players will carry over their inventory in Co-op. Default `1`

## Notes

* This entity works in multiplayer, unlike in the original SDK
* The game will disable level changes if the player is touching them immediately after entering the map. A message will be shown on-screen and in the console to warn the player. The original behavior disabled all level changes in the map