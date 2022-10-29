# Client Command Registry

The Half-Life Unified SDK provides a means of defining client commands that can be added and removed at any time. This is in contrast to the original SDK where client commands are handled by a chain of if statements.

## Command structure

Commands take a `std::function` object for a function signature `void(CBasePlayer* player, const CCommandArgs& args)`. Typical use involves using a capturing lambda to forward command execution to a member function, as shown below.

The player entity is that of the player who issued the command.

Commands also have a name and flags. The name must include only lowercase alphabetical and underscore characters.

### Flags

* `ClientCommandFlag::None`: Constant to use when no flags are set
* `ClientCommandFlag::Cheat`: Indicates that this command is a cheat command, only allowed when `sv_cheats is non-zero`

## Registering commands

There are two ways to register commands, depending on how you want to manage the lifetime of the command.

If a command exists for the lifetime of another object like a gamerules class then you should use a scoped command:
```cpp
//In the class declaration:
CScopedClientCommand m_SpectateCommand;

//In the constructor:
CGameRules::CGameRules()
{
	m_SpectateCommand = g_ClientCommands.CreateScoped("spectate", [this](CBasePlayer* player, const CCommandArgs& args)
		{
			// clients wants to become a spectator
			BecomeSpectator(player, args);
		});
}
```

The command will be removed automatically when the gamerules object is destroyed.

When the command lifetime does not matter or if you need to manage its lifetime manually you should use the `Create` function:
```cpp
g_ClientCommands.Create("say", [](CBasePlayer* player, const CCommandArgs& args)
	{
		Host_Say(player->edict(), false);
	});
```

The shared pointer returned by this function can be used to remove the command at any time by calling `Remove`:
```cpp
mySharedPointer->Registry->Remove(mySharedPointer);
mySharedPointer.reset(); //Ensure the command is destroyed.
```

To set flags on the command, pass the flags as an additional argument:
```cpp
g_ClientCommands.Create("give", [](CBasePlayer* player, const CCommandArgs& args)
	{
		int iszItem = ALLOC_STRING(args.Argument(1)); // Make a copy of the classname
		player->GiveNamedItem(STRING(iszItem));
	},
	{.Flags = ClientCommandFlag::Cheat});
```

This feature relies on [Designated initializers](https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers).
