# SourceFileFormatter

> **Note**
> This is a [dotnet script tool](/docs/tutorials/setting-up-and-using-dotnet-script.md)

> **Note**
> This tool is included in the `scripts` directory in the [SDK repository](https://github.com/SamVanheer/halflife-unified-sdk)

The SourceFileFormatter script is used to run clang-format on all of the files in a directory matching a set of filters.

## Predefined scripts

Two scripts are provided that will format all source files (filters `*.h` and `*.cpp`) in the SDK repository (directories `src` and `utils`); one for Windows and one for Linux:

* Windows: `scripts/format_all_source_files.bat`
* Linux: `scripts/format_all_source_files.sh`

Use these scripts to format source files prior to committing changes. You can also use Visual Studio's built-in ClangFormat support to format files.

## Usage

### Command line

```
Usage:
  dotnet script "path/to/SourceFileFormatter.csx" [<directories>...] [options]

Arguments:
  <directories>  Directories to scan for files to format

Options:
  --filter <filter> (REQUIRED)           Wildcard patterns to filter files with. Repeat this option to specify multiple
                                         patterns
  --clang-format-exe <clang-format-exe>  Path to the clang-format executable. Defaults to the path provided by the
                                         CLANG_FORMAT environment variable [default: <varies depending on user>]
  --version                              Show version information
  -?, -h, --help                         Show help and usage information
```

The destination directory should point to the `cfgsrc/maps` directory in your local copy of the [assets repository](https://github.com/SamVanheer/halflife-unified-sdk-assets).

## Providing a ClangFormat executable to use

By default the tool will use the executable specified by the `CLANG_FORMAT` environment variable.

It is recommended to use the same executable used by Visual Studio. Visual Studio ships a ClangFormat executable used when projects have a `.clang-format` file. This executable is located in `path/to/VisualStudio\<Visual Studio version>\<Visual Studio edition>\VC\Tools\Llvm\x64\bin\clang-format.exe`.

A possible location for this is `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe`.

Visual Studio allows users to override this with a custom ClangFormat executable. If you are using a custom executable then you should use the same one with this script.

This option is located in `Tools->Options...->Text Editor->C/C++->Code Style->Formatting->General` and is called `Use custom clang-format.exe file`.

### More information

[Adding environment variables on Windows](https://docs.oracle.com/en/database/oracle/machine-learning/oml4r/1.5.1/oread/creating-and-modifying-environment-variables-on-windows.html)

[Adding environment variables on Linux](https://askubuntu.com/a/58828)

[Visual Studio ClangFormat documentation](https://learn.microsoft.com/en-us/visualstudio/ide/reference/options-text-editor-c-cpp-formatting?view=vs-2022#configuring-clangformat-options)

## Error reporting

If any errors occur the script will log it to the console.
