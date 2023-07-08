# Material System

The Half-Life Unified SDK has an overhauled material system that replaces the singular `materials.txt` with a JSON-based configuration file that supports multiple map-specific materials files.

The default file loaded by default is `sound/materials.json`. Additional files can be added using map configuration files.

The material system uses the logger named `materials`.

## Syntax

A materials file contains an object whose keys are texture names. Each entry is an object containing the material type for that texture.

Unlike `materials.txt` the texture name is not limited to **12** characters. Texture names can be up to **15** characters, matching the limits in WAD and BSP files.

Note that texture names should not include the following prefixes:
* Random texture prefix (`-0` and alternates)
* Animated texture prefix (`+0`, `+A` and alternates)
* `'{'`
* `'!'`
* `'~'`
* `' '` (single whitespace character)

### Example

```jsonc
{
	"DUCT_FLR01": {
		"Type": "V"
	},
	"OUT_GRND1": {
		"Type": "D"
	}
	//"OUT_RK1": {
	//	"Type": "D"
	//},
}
```

## See Also

* [Materials section](game-configuration-system.md#materials)
