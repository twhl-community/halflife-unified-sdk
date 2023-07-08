# Inventory System

The Half-Life Unified SDK allows level designers to have greater control over the players' inventories.

`trigger_changelevel` now has the option to enable persistent inventory in Co-op. All players in the server will carry over their inventory to the next level allowing for smoother transitions.

The `coop_persistent_inventory_grace_period` skill variable controls how long after the player has finished connecting the player is able to receive their inventory. This allows them to re-acquire their inventory even if they die after spawning in the next map.

it is also possible to specify the inventory that players (re)spawn with by adding it to the map configuration file.

This can be done in all game modes and is used to give players the default loadout in multiplayer.

## What is stored in the inventory

The following data is stored in the inventory:
* Whether the player has the HEV suit
* Whether the player has the long jump module
* Health
* Armor
* Weapons (including current magazine contents and any weapon-specific data that is carried over)
* Reserve ammo
* Which weapon the player has equipped

All but weapon-specific data can be configured in the spawn inventory configuration.

## See Also

* [SpawnInventory section](game-configuration-system.md#spawninventory)
