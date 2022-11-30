#r "nuget: HalfLife.UnifiedSdk.Utilities, 0.1.4"
#r "nuget: Microsoft.Extensions.FileSystemGlobbing, 6.0.0"
#r "nuget: System.CommandLine, 2.0.0-beta4.22272.1"

#nullable enable

using HalfLife.UnifiedSdk.Utilities.Logging;
using Microsoft.Extensions.FileSystemGlobbing;
using System.CommandLine;
using System.Diagnostics;

const string ClangFormatEnvKey = "CLANG_FORMAT";

var directoriesArgument = new Argument<IEnumerable<DirectoryInfo>>("directories", description: "Directories to scan for files to format");

var filtersOption = new Option<IEnumerable<string>>("--filter", description: "Wildcard patterns to filter files with. Repeat this option to specify multiple patterns")
{
	IsRequired = true
};

var defaultClangFormat = new FileInfo(Environment.GetEnvironmentVariable(ClangFormatEnvKey) ?? string.Empty);

var clangFormatExeOption = new Option<FileInfo>("--clang-format-exe",
	getDefaultValue: () => defaultClangFormat,
	description: $"Path to the clang-format executable. Defaults to the path provided by the {ClangFormatEnvKey} environment variable");

var rootCommand = new RootCommand("Half-Life Unified SDK Source file formatter")
{
	directoriesArgument,
	filtersOption,
	clangFormatExeOption
};

rootCommand.SetHandler((directories, filters, clangFormatExe, logger) =>
{
	if (!clangFormatExe.Exists)
	{
		logger.Error("The clang-format executable \"{FileName}\" does not exist", clangFormatExe.FullName);
		return;
	}

	var clangFormatExeFileName = clangFormatExe.FullName;

	var matcher = new Matcher();

	matcher.AddIncludePatterns(filters);
	
	logger.Information("Formatting files");
	
	int count = 0;

	foreach (var directory in directories)
	{
		if (directory.Exists)
		{
			foreach (var file in matcher.GetResultsInFullPath(directory.FullName))
			{
				Process.Start(clangFormatExeFileName, new[]{ "-i", file });
				++count;
			}
		}
		else
		{
			logger.Error("The given directory \"{Directory}\" does not exist", directory);
		}
	}
	
	logger.Information("Formatted {FileCount} files in {DirectoryCount} directories matching {FilterCount} filters", count, directories.Count(), filters.Count());
}, directoriesArgument, filtersOption, clangFormatExeOption, LoggerBinder.Instance);

return await rootCommand.InvokeAsync(Args.ToArray());
