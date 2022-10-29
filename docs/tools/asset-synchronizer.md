# AssetSynchronizer

The AssetSynchronizer tool is used to automatically copy assets from the assets repository to the game installation.

This tool is included in the `hlu/tools` directory in the game installation package available here: https://github.com/SamVanheer/halflife-unified-sdk/releases

# Purpose

When executed this tool copies any files that are different from the assets repository to the mod installation.
While active it monitors the files and copies them automatically on change, creation or rename.

Files are not deleted from the destination directory.

**Note**: make sure to make regular backups of your files. This tool can and will overwrite files without warning, and you will not be able to recover the file's previous contents.

# Prerequisites

## Install the .NET SDK

See [Setting up the .NET SDK](/docs/tutorials/setting-up-dotnet-sdk.md)

# Usage

## Command line

```
dotnet "path/to/HalfLife.UnifiedSdk.AssetSynchronizer.dll"
--assets-directory "path/to/assets/repository"
--mod-directory "path/to/half-life/unified/sdk/mod"
--asset-manifest "path/to/AssetManifest.json"
```

The assets directory should point to your local copy of the [assets repository](https://github.com/SamVanheer/halflife-unified-sdk-assets).

The mod directory should point to your local copy of the [mod installation](/INSTALL.md).

The asset manifest should point to the asset manifest file contained in the assets repository.

It is recommended to use a script to pass the arguments automatically. For example, using a batch file placed in the directory containing the assets repository:
```bat
dotnet "path/to/HalfLife.UnifiedSdk.AssetSynchronizer.dll"^
	--assets-directory "halflife-unified-sdk-assets"^
	--mod-directory "C:/Program Files (x86)/Steam/steamapps/common/Half-Life/hlu"^
	--asset-manifest "halflife-unified-sdk-assets/AssetManifest.json"
```

You can then execute the batch file without having to provide the arguments every time.

# Error reporting

If any errors occur the tool will log it to the console. Typical errors are files being read-only, attempting to copy a file when a directory with the same name exists, or files having security settings prohibiting copying or overwriting.

# AssetManifest.json format

The asset manifest file contains a list of filters used to control which directories and files are copied.

The basic format is a JSON array:
```jsonc
[
]
```

Each array element is an object containing these properties:
```jsonc
{
	"Source": "cfgsrc",
	"Destination": "cfg",
	"Pattern": "*.json",
	"Recursive": true
}
```

`Source` is the relative path to the directory in the assets directory.
`Destination` is the relative path to the directory in the game directory.
`Pattern` is a [wildcard pattern](https://en.wikipedia.org/wiki/Wildcard_character#File_and_directory_patterns) used to filter for specific files in a directory.
`Recursive` controls whether subdirectories are also checked. If not specified this defaults to `false`.

Multiple filters can have the same source and destination, but it is advised not to use the same patterns as this will cause the same file to be copied multiple times.

To specify a file in the assets or game directory, specify an empty string for `Source` or `Destination`.

Example:
```jsonc
//This file contains a list of filters to copy assets to the game directory.
[
	{
		"Source": "cfgsrc",
		"Destination": "",
		"Pattern": "server.cfg",
	},
	{
		"Source": "cfgsrc",
		"Destination": "cfg",
		"Pattern": "*.json",
		"Recursive": true
	},
	{
		"Source": "cfgsrc",
		"Destination": "",
		"Pattern": "*.scr.install",
	},
	{
		"Source": "cfgsrc",
		"Destination": "",
		"Pattern": "titles.txt"
	},
	{
		"Source": "resourcesrc",
		"Destination": "resource",
		"Pattern": "*.txt",
		"Recursive": true
	},
	{
		"Source": "soundsrc",
		"Destination": "sound",
		"Pattern": "*.txt"
	}
]
```
