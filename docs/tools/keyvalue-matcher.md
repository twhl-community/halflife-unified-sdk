# KeyValueMatcher

> **Note**
> This is a [.NET tool](../dotnet-tools.md)

> **Note**
> This tool is included in the `hlu/tools` directory in the Releases available [here](../README.md#developer-resources)

The KeyValueMatcher tool is used to scan all of the BSP files in a directory for specific patterns.

## Usage

### Command line

```
Usage:
  KeyValueMatcher [<maps-directory>...] [options]

Arguments:
  <maps-directory>  One or more paths to the maps directories to search

Options:
  --print-mode <Entity|KeyValue|Nothing>    What to print when a match is found [default: KeyValue]
  --classname <classname>                   Classname regex pattern
  --key <key>                               Key regex pattern
  --value <value>                           Value regex pattern
  --flags-name <flags-name>                 If specified, name of the flags key to match (e.g. spawnflags)
  --flags-match-mode <Exclusive|Inclusive>  How to match flags [default: Inclusive]
  --flags <flags>                           Flag value to match [default: 0]
  --version                                 Show version information
  -?, -h, --help                            Show help and usage information
```

Regular expressions are used to match against entity data. If a pattern isn't provided then it will be treated as though it matches everything (`.*`).

## Error reporting

If any errors occur the tool will log it to the console and stop execution.
