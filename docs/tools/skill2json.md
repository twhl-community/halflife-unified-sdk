# Skill2Json

> **Note**
> This is a [.NET tool](/docs/dotnet-tools.md)

> **Note**
> This tool is included in the `hlu/tools` directory in the game installation package available [here](https://github.com/SamVanheer/halflife-unified-sdk/releases)

The Skill2Json tool is used to convert Half-Life 1 `skill.cfg` files to the Unified SDK's `skill.json` format.

## Usage

### Command line

```
dotnet "path/to/HalfLife.UnifiedSdk.Skill2Json.dll"
--output-filename "path/to/destination/file.json"
<filename>
```

The `output-filename` parameter specifies where to save the converted file. If not specified the file is saved to the source directory with the same name and `json` extension.

Specify a cfg file to convert.

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
