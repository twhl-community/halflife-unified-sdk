# Logging System

The Half-Life Unified SDK uses [spdlog](https://github.com/gabime/spdlog) to handle logging in new systems.

Because the logging system is implemented at the SDK level a separate version exists for the client and server, so all commands are prefixed with the library name to disambiguate them.

File logging is supported. If file logging is enabled a log file will be created in `logs/<shortlibraryprefix>` where depending on whether the client or server is logging `shortlibraryprefix` is either `cl` or `sv`.

Log filenames are of the form `<BaseFileName>_YYYY_MM_DD.log`.

Console log output is of the form:
`[logger_name] [log_level] message`

File log output is of the form:
`[2022-06-02 14:27:17.148] [logger_name] [log_level] message`

The logging system uses the logger named `logging`.

## Log levels

From most to least verbose:

* trace
* debug
* info
* warning
* error
* critical
* off

## List of loggers

| Name | Purpose |
| --- | --- |
| angelscript | See [Angelscript Scripting Support](/angelscript-scripting-support.md) |
| assert | In debug builds this logs debug assertion messages using the `critical` level |
| conditional_evaluation | See [Angelscript-scripting-support](/#conditional-evaluation-api) |
| cvar | See [Console Command System](console-command-system.md) |
| ent | Logs general entity info |
| ent.ai | Logs NPC info |
| ent.ai.script | Logs NPC scripted behavior info (`scripted_sequence`, `aiscripted_sequence` & `scripted_sentence`) |
| ent.io | Logs entity I/O related to target and killtarget |
| ent.weapons | Logs weapon state info |
| game | Logs general game events related to map loading and initialization |
| gamecfg | See [Game Configuration System](game-configuration-system.md) |
| gamerules | Logs game mode events. This includes log output that was previously routed to the server log file |
| json | See [JSON System](json-system.md) |
| logging | See above |
| nodegraph | Logs node graph debug output. Will spew much output in trace mode |
| precache | Logs every call made to model, sound and generic precache functions. Assets precached by the engine are not included in this output |
| replacementmap | See [Replacement Map System](replacement-map-system.md) |
| saverestore | Logs errors that occur during saving and loading |
| sentences | Logs diagnostics, warnings and errors that occur during `sentences.txt` file loading |
| skill | See [Skill Configuration System](skill-configuration-system.md) |
| sound | Logs diagnostics, warnings and errors that occur during sound playback |

## Error handling

If an error occurs during logging an error message will be printed:
> [sv|cl] [spdlog] [error]: &lt;error message&gt;

This should only happen if a mistake was made in the game's C++ code. A developer should investigate these errors and fix them.

## Configuration file syntax

`cfg/logging.json` is a JSON file containing settings for loggers and file logging.

### Keys

#### Reset

Type: boolean

If true, any previous settings will be reset to their defaults before the configuration file is applied.

#### Defaults

Type: object

Default values for all loggers.

Subkeys:

| Name | Type | Purpose |
| --- | --- | --- |
| LogLevel | string | The default log level for all loggers, unless overridden on a logger-specific basis |

#### LoggerConfigurations

Type: array

A list of objects containing settings for specific loggers.

Subkeys:

| Name | Type | Purpose |
| --- | --- | --- |
| Name | string | The name of the logger. See the `log_listloggers` console command for a list of loggers |
| LogLevel | string | The log level for this logger |

#### LogFile

Type: object

Settings for file logging.

Subkeys:

| Name | Type | Purpose |
| --- | --- | --- |
| Enabled | boolean | Whether file logging is enabled |
| BaseFileName | string | The base filename for the log filename. Defaults to `L` |
| MaxFiles | integer | The maximum number of log files to create before deleting old files. Note that this indicates the maximum number of contiguous files. If no log file is created for a day then files created before that day will not be counted |

### Example

```jsonc
//Configuration file for Half-Life Unified SDK logging system.
{
	//If set to true, all previous logging settings will be reset before this configuration is applied.
	//This will reset command line settings.
	"Reset": false,
	
	//Default logger configuration. Used for every logger unless overridden.
	"Defaults": {
		"LogLevel": "info"
	},
	//Logger-specific configuration.
	"LoggerConfigurations": [
		//Sample configuration. Use the sv_log_list_loggers command to get a list of logger names.
		//{
		//	"Name": "logging",
		//	"LogLevel": "trace"
		//}
	],
	//Configuration for logging to a file.
	"LogFile": {
		"Enabled": false,
		"BaseFileName": "Unified",
		"MaxFiles": 8
	}
}
```

This configuration file inherits existing settings, sets the default log level for all loggers to `info` and disables file logging.

## Console commands

Since the logging system is available on both client and server all commands are prefixed with either `cl_` or `sv_`.

### log_listloglevels

Syntax: `log_listloglevels`

Prints a list of all log levels that loggers can be set to. They are listed from most to least verbose.

### log_listloggers

Syntax: `log_listloggers`

Prints a list of all loggers.

### log_setlevel

Syntax: `log_setlevel logger_name [log_level]`

When executed with only the logger name the current log level is printed.
When executed with the log level it will set the logger's log level to that level.

### log_setalllevels

Syntax: `log_setalllevels log_level`

Sets all loggers to use the given log level.

### log_setentlevels

Syntax: `log_setentlevels log_level`

Sets all loggers with the `ent` prefix as well as `nodegraph`, `saverestore`, `gamerules` and `voice` to the given log level. This is the functional equivalent of the `developer` cvar as it affects all log output that was previously affected by that cvar.

### log_file

Syntax: `log_file < on | off >`

When executed without arguments the file logging state is printed.
When executed with either `on` or `off` file logging is enabled or disabled.

## Command line parameters

### -log_startup_level

Syntax: `-log_startup_level <log_level>`

Sets the default log level during startup. The configuration file will override this setting.

Used mainly for debugging early startup code.

If not specified the default log level depends on the value of the `developer` cvar. If it is greater than zero the `debug` level is used, otherwise `info` is used.

Note that the only way to change the `developer` cvar this early is to use the `-dev` command line parameter.

### -log_file_on

Syntax: `-log_file_on`

Enables file logging on startup. The configuration file will override this setting.
