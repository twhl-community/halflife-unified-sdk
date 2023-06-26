# Testing Guidelines

* Save and load save games right after starting a map to catch leftover state and state that should be saved but isn't
* Test multiplayer with bots

## Map-specific guidelines

* c3a2d node graph generates incorrectly when the map is loaded through level change. Doesn't happen consistently, might be the result of global state carrying over through several maps