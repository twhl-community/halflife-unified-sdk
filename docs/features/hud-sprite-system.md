# Hud Sprite System

The Half-Life Unified SDK overhauls HUD sprite configuration files to use JSON and adds the ability to replace the files used on a map-to-map basis.

## Behavior

The client loads HUD sprite configuration files when a map is loaded. `sprites/hud.json` defines the majority of the sprites, used by most of the HUD.

The weapon HUD loads a configuration file for each weapon. By default this is `sprites/<classname>.json` where `<classname>` is the classname of the weapon entity, for example `weapon_9mmar`.

Weapons define a set of HUD sprites specific to that weapon such as the weapon list icon, crosshairs and ammo types.

HUD replacement works by specifying alternate files to use. Instead of loading the default file the file specified in the map configuration file is used, which allows the HUD to be customized to some degree.

For example the Opposing Force campaign replaces many sprites with variants that have scanline effects and uses digital-looking numbers.

## Syntax

A HUD configuration file is an object mapping HUD sprite names to the actual sprite name and specifying the rectangle within that sprite that represents the HUD sprite.

### HUD Sprite Properties

| Name | Type | Purpose |
| --- | --- | --- |
| SpriteName | string | Name of the file in the `sprites` directory, excluding file extension. If the file is located in a subdirectory then that directory needs to be part of the filename. |
| Left | integer | X coordinate of the HUD sprite in the sprite sheet |
| Top | integer | Y coordinate of the HUD sprite in the sprite sheet |
| Width | integer | Width of the HUD sprite in the sprite sheet |
| Height | integer | Height of the HUD sprite in the sprite sheet |

### Weapon Configuration Properties

| Name | Purpose |
| --- | --- |
| crosshair | The unzoomed crosshair |
| autoaim | The unzoomed crosshair when autoaim has locked onto a target |
| zoom | The zoomed crosshair |
| zoom_autoaim | The zoomed crosshair when autoaim has locked onto a target |
| weapon | The weapon list and history list icon |
| weapon_s | The weapon list icon when the player has selected the weapon |
| ammo | The ammo icon for the first weapon ammo type |
| ammo2 | The ammo icon for the second weapon ammo type |

### Example

```jsonc
{
	"selection": {
		"SpriteName": "hud3",
		"Left": 0,
		"Top": 180,
		"Width": 170,
		"Height": 45
	},
	"bucket1": {
		"SpriteName": "op4/hud3_scanline",
		"Left": 170,
		"Top": 120,
		"Width": 20,
		"Height": 20
	}
}
```

## See Also

* [HudReplacement section](game-configuration-system.md#hudreplacement)
