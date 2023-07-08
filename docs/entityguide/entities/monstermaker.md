# monstermaker

**Point entity**

Spawns NPCs and other entities.

> <span style="background-color:darkseagreen; color: black">
> Note
></br>
> Only changes are listed in this documentation. Consult TWHL's page for more information.</span>

[TWHL:monstermaker](https://twhl.info/wiki/page/monstermaker)

## Keyvalues

* **#&lt;anything&gt;** - Any key prefixed with `#` will be passed directly to spawned entities. Up to **64** keyvalues can be set this way

## Note

* The Unified SDK triggers the Fire On Release (`target`) keyvalue after the NPC has been spawned as opposed to the original SDK which fires it before the NPC has been given its targetname
