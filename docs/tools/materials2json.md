# Materials2Json

> **Note**
> This is a [.NET tool](../dotnet-tools.md)

> **Note**
> This tool is included in the `hlu/tools` directory in the Releases available [here](../README.md#developer-resources)

The Materials2Json tool is used to convert Half-Life 1 `sound/materials.txt` files to the Unified SDK's `materials.json` format.

## Usage

### Command line

```
Usage:
  Materials2Json <filename> [options]

Arguments:
  <filename>  materials to convert

Options:
  --output-filename <output-filename>  If provided, the name of the file to write the materials.json contents to.
                                       Otherwise the file is saved to the source directory with the same name and
                                       'json' extension. []
  --version                            Show version information
  -?, -h, --help                       Show help and usage information
```

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
