# trigger_playerfreeze

**Point entity**

Point entity that freezes/unfreezes players when triggered. Trigger to freeze, again to unfreeze. This can be repeated as many times as necessary. The entity tracks whether to freeze and unfreeze; it does not toggle the player's current frozen state.

## Keyvalues

* **all_players** - Whether to affect all players or the activating player

## Notes

* Other entities like `trigger_camera` can also freeze and unfreeze players. Be careful when setting up scripts to prevent players from being unfrozen too early.
