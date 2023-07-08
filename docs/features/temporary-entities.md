# Temporary Entities

The Half-Life Unified SDK replaces the engine's temporary entities list with a new one that raises the maximum number of temporary entities from **500** to **2048** and can be further increased by changing the `MAX_TEMPENTS` constant in `cdll_dll.h`.

This is a client-side list of entities used for purely visual effects like debris, gibs, models attached to the player and so on.

The new list is implemented by overriding the engine's interface for creating temporary entities. The engine itself also uses this interface to create them which allows existing engine code to continue working.

Additionally a performance issue caused by the logging of the warning `Overflow 2048 temporary ents!` (**500** in the engine's version) has been fixed.

Note that while the maximum number of entities has been increased the engine's limit of **512** visible entities still applies. Without a custom renderer temporary entities will become invisibile if there are too many in the same area.
