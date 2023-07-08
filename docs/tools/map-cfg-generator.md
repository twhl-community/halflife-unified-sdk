# MapCfgGenerator

> **Note**
> This is a [.NET tool](../dotnet-tools.md)

> **Note**
> This tool is included in the `hlu/tools` directory in the Releases available [here](../README.md#developer-resources)

The MapUpgrader tool is used to generate map configuration files for official Half-Life, Opposing Force and Blue Shift maps.
These files all have the same contents for a particular game.

## Usage

### Command line

```
Usage:
  MapCfgGenerator <destination-directory> [options]

Arguments:
  <destination-directory>  Where to put the generated files

Options:
  --version       Show version information
  -?, -h, --help  Show help and usage information
```

The destination directory should point to the `cfgsrc/maps` directory in your local copy of the [assets repository](../README.md#developer-resources).

## Error reporting

If any errors occur the tool will log it to the console.

## Sample configuration file contents

Possible contents for a Half-Life map are:
```jsonc
{
	"Includes": [
		"cfg/HalfLifeConfig.json"
	]
}
```

The included file depends on the game the map is a part of.
