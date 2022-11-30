# Installer

> **Note**
> This is a [.NET tool](/docs/dotnet-tools.md)

> **Note**
> This tool is included in the `hlu/tools` directory in the game installation package available [here](https://github.com/SamVanheer/halflife-unified-sdk/releases)

The Installer tool is used to copy, convert and upgrade Half-Life game assets for a mod installation.
This will copy campaign, training and multiplayer maps and apply any necessary upgrades and bug fixes.

## Usage

### Command line

```
Usage:
  HalfLife.UnifiedSdk.Installer [options]

Options:
  --mod-directory <mod-directory>            Path to the mod directory
  --dry-run                                  If provided no file changes will be written to disk
  --diagnostics-level <All|Common|Disabled>  The diagnostics level to set [default: Disabled]
  --version                                  Show version information
  -?, -h, --help                             Show help and usage information
```

The mod directory should point to your local copy of the [mod installation](/INSTALL.md).

You can optionally provide `--dry-run` to see which files would be copied without actually making any changes.

It is recommended to use a script to pass the arguments automatically. For example, using a batch file placed in the directory containing the game installation (typically `Half-Life`):
```bat
dotnet "path/to/HalfLife.UnifiedSdk.Installer.dll"^
	--mod-directory "C:/Program Files (x86)/Steam/steamapps/common/Half-Life/hlu"
```

You can then execute the batch file without having to provide the arguments every time.

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
