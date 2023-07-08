# Angelscript Scripting Support

The Half-Life Unified SDK uses [Angelscript](https://www.angelcode.com/angelscript) for some scripting functionality.

Angelscript scripting support uses the logger named `angelscript`.

## Conditional evaluation

Some configuration files use conditional evaluation to include settings. This is done using Angelscript.

A string containing a conditionally evaluated expression is compiled and executed as Angelscript code by wrapping it in a function:
```cs
bool Evaluate()
{
	return condition;
}
```

Conditional evaluation will time out if it takes longer than 1 second to execute.

### Conditional evaluation API

This API is available for use in conditional evaluation.

#### Constants

* `Singleplayer`: `true` if the game is running in singleplayer mode
* `Multiplayer`: `true` if the game is running in multiplayer mode
* `ListenServer`: `true` if the server is hosted by a player
* `DedicatedServer`: `true` if the server is hosted as a dedicated server
* `GameMode`: Name of the current game mode. See [Game Modes](game-modes.md#available-game-modes)
