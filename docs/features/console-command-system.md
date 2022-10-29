# Console Command System

To facilitate the writing of code that is used on both the client and server side the Half-Life Unified SDK has an abstraction for console commands to enable this without having to conditionally compile code with `#ifdef CLIENT_DLL`.

On the client side commands are prefixed with `cl`; on the server side commands are prefixed with `sv`. This is known as the library prefix.

The console command system uses the logger named `cvar`.

## Registering commands

Registering a command is straightforward enough:
```cpp
g_ConCommands.CreateCommand("command_name", [this](const auto& args) { MyMemberFunction(args); });

void CMyClass::MyMemberFunction(const CCommandArgs& args)
{
	if (args.Count() > 1)
	{
		Con_Printf("%s\n", args.Argument(1));
	}
}
```

Commands take a `std::function` object for a function signature `void(const CCommandArgs& args)`. Typical use involves using a capturing lambda to forward command execution to a member function, as shown above.

You can also disable the use of a library prefix:
```cpp
g_ConCommands.CreateCommand("command_name", [this](const auto& args) { MyMemberFunction(args); }, CommandLibraryPrefix::No);
```

## Registering cvars

Registering a cvar is even simpler:
```cpp
m_MyCvar = g_ConCommands.CreateCVar("my_cvar", "0", FCVAR_PROTECTED);
```

You can set the initial value and flags.

You can also disable the use of a library prefix:
```cpp
m_MyCvar = g_ConCommands.CreateCVar("my_cvar", "0", FCVAR_PROTECTED, CommandLibraryPrefix::No);
```

## Setting cvars on the command line

CVars registered with this system can be set from the command line in one of two ways.

Both ways require you to prefix the cvar name with `:`.

The first is to use the complete cvar name:
```
:sv_cvar_name argument
```

The second is to use only the base name without the library prefix:
```
:cvar_name argument
```

This allows you to set both the client and server version of a cvar at the same time.

CVar values should be quoted if they have spaces or if they start with a `+` or `-`.
