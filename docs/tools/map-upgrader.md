# MapUpgrader

The MapUpgrader tool is used to upgrade Half-Life maps for a mod installation.

This tool is included in the `hlu/tools` directory in the game installation package available here: https://github.com/SamVanheer/halflife-unified-sdk/releases

## Purpose

When executed this tool upgrades Half-Life maps for a mod installation.
This will apply any necessary upgrades and bug fixes.

## Prerequisites

### Install the .NET SDK

See [Setting up the .NET SDK](/docs/tutorials/setting-up-dotnet-sdk.md)

## Usage

### Command line

```
dotnet "path/to/HalfLife.UnifiedSdk.MapUpgrader.dll"
--game "gameModDirectoryName"
--maps "path/to/map.bsp" ["path/to/other_map.bsp"] [...]
```

The game parameter is the mod directory name of the game the maps originate from. If not specified, the map will be treated as a vanilla Half-Life 1 map.

Specify one or more paths to maps to be upgraded.

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
