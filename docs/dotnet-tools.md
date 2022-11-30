# .NET Tools

The Unified SDK uses tools written in .NET. These tools are intended to work on Windows and Linux.

## Prerequisites

### Install the .NET SDK

.NET tools require the .NET SDK.

See [Setting up the .NET SDK](/docs/tutorials/setting-up-dotnet-sdk.md)

## Launching .NET tools

All tools include a Windows executable (`.exe`) and a .NET assembly (`.dll`).

Launching the executable is as simple as double-clicking it or running it in a command window:
```cmd
path/to/tools/HalfLife.UnifiedSdk.ToolName.exe [options] <arguments>
```

For use on other platforms the assembly should be used:
```cmd
dotnet path/to/tools/HalfLife.UnifiedSdk.ToolName.dll [options] <arguments>
```

## Command-line tools

All command-line tools use [System.CommandLine](https://learn.microsoft.com/en-us/dotnet/standard/commandline/) to handle user input.

This means all command-line tools have the following options:

### version

Shows version information.

### -?, -h, --help

Shows help and usage information.

Use this option to get more information about a tool's purpose and available functionality.
