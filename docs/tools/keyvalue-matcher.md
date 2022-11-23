# KeyValueMatcher

The KeyValueMatcher tool is used to scan all of the BSP files in a directory for specific patterns.

This tool is included in the `hlu/tools` directory in the game installation package available here: https://github.com/SamVanheer/halflife-unified-sdk/releases

## Purpose

When executed this tool scans BSP files for specific patterns.

## Prerequisites

### Install the .NET SDK

See [Setting up the .NET SDK](/docs/tutorials/setting-up-dotnet-sdk.md)

## Usage

### Command line

```
dotnet "path/to/HalfLife.UnifiedSdk.KeyValueMatcher.dll" [options]
```

#### Options:

```
--maps-directory <maps-directory> (REQUIRED)  Path to the maps directory to search
--print-mode <Entity|KeyValue|Nothing>        What to print when a match is found [default: KeyValue]
--classname <classname>                       Classname regex pattern
--key <key>                                   Key regex pattern
--value <value>                               Value regex pattern
```

Regular expressions are used to match against entity data. If a pattern isn't provided then it will be treated as though it matches everything (`.*`).

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
