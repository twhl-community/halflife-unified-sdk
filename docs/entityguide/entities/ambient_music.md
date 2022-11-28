## ambient_music

**Point entity**

This entity allows level designers to control music playback.

You can play and loop music, trigger it to fade out or stop it entirely.

### Attributes

* **filename** - Relative name of the file to play. Only applies to the `Play` and `Loop` commands
* **command** - The command to issue. Any of `Play, Loop, Fadeout, Stop`
* **target_selector** - How to select players to affect. Any of `AllPlayers, Activator, Radius`
* **radius** - If `target_selector` is `Radius`, this is how close the player has to be to be affected by this entity
* **remove_on_fire** - Whether to remove the entity after it has been triggered or after it has affected at least one player (`Radius` only)

### Notes

* Part of the [OpenAL sound system](/docs/features/sound-system.md)
