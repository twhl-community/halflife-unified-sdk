# Bsp2Obj

The Bsp2Obj tool is used to convert Half-Life 1 BSP files to the Wavefront OBJ format.

This tool is included in the `hlu/tools` directory in the game installation package available here: https://github.com/SamVanheer/halflife-unified-sdk/releases

## Purpose

When executed this tool converts BSP files to OBJ files.

The following files are generated:
* `mapname.obj`
* `mapname.mtl`
* `mapname_material_index.tga`

`mapname` is the name of the BSP file. `index` is the number of the generated material.

Materials are only generated if the texture is embedded in the BSP file, otherwise a dummy material with no texture is used.

## Prerequisites

### Install the .NET SDK

See [Setting up the .NET SDK](/docs/tutorials/setting-up-dotnet-sdk.md)

## Usage

### Command line

```
dotnet "path/to/HalfLife.UnifiedSdk.Bsp2Obj.dll"
--destination "path/to/destination/directory"
<bsp filename>
```

The `destination` parameter specifies where to save generated files. If not specified the files are saved in the same directory as the BSP file.

Specify a BSP file to convert.

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
