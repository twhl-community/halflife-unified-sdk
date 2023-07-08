# Game Modes

The Half-Life Unified SDK overhauls multiplayer game mode selection to allow greater control over the current game mode for both server operators and level designers.

Game modes use the logger named `gamerules`.

## Behavior

Level designers can choose the default game mode to use by setting it in the map configuration file. They can also lock the game mode to prevent the server operator from changing it (e.g. for Co-op maps).

Server operators can set the game mode by setting the `mp_gamemode` cvar to the name of a game mode. Set this cvar to an empty string to automatically select the appropriate game mode.

Due to a limitation of the `Create Server` dialog a separate cvar called `mp_createserver_gamemode` is used to set the game mode through this dialog.

This cvar is set to an empty string once the server has started and the `mp_gamemode` cvar is set to the selected game mode.

If an invalid game mode is specified the `deathmatch` game mode is used.

In singleplayer the singleplayer game mode is always used regardless of the game mode setting.

## Available game modes

| Name | Description |
| --- | --- |
| singleplayer | Singleplayer. Cannot be used in the game mode selection options |
| deathmatch | Free-for-all deathmatch |
| teamplay | Player model-based team deathmatch |
| ctf | Capture The Flag |
| coop | Co-operative play (players versus enemies) |

The list of multiplayer game modes can also be retrieved using the console command `mp_list_gamemodes` when loaded into a local game.

## See Also

* [Game Mode Properties](game-configuration-system.md#game-mode-properties)
