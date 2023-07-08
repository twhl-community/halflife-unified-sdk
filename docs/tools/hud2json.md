# Hud2Json

> **Note**
> This is a [.NET tool](../dotnet-tools.md)

> **Note**
> This tool is included in the `hlu/tools` directory in the Releases available [here](../README.md#developer-resources)

The Hud2Json tool is used to convert Half-Life 1 `sprites/hud.txt` and `sprites/weapon_*.txt` files to the Unified SDK's `hud.json` format.

## Usage

### Command line

```
Usage:
  Hud2Json <filename> [options]

Arguments:
  <filename>  hud definition to convert

Options:
  --output-filename <output-filename>  If provided, the name of the file to write the hud.json contents to.
                                       Otherwise the file is saved to the source directory with the same name and
                                       'json' extension. []
  --version                            Show version information
  -?, -h, --help                       Show help and usage information
```

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
