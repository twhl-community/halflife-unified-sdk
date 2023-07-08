# Bot system

The Half-Life Unified SDK provides a barebones bot system meant to be used for basic multiplayer testing.

Bots are players that use AI to control behavior. Each bot takes up one player slot in the server. As such they are only available in multiplayer.

The bots provided in the SDK are only capable of spawning and updating their movement based on external factors like gravity and entities that move them (e.g. `trigger_push`).

This allows you to test weapons against them to see how players respond to injuries and death.

Bots are not capable of joining teams in the Capture The Flag gamemode without additional code changes.

Bots can be removed from the server by kicking them as you would kick a player with the `kick` command.

The bot system uses the logger named `bot`.

## Console commands

### sv_addbot

Syntax: `sv_addbot <bot name>`

Adds a bot with the given name. If a player with that name already exists the engine will add a `(<number>)` prefix to the name.
