# Using FastDL with servers running Unified SDK mods

For the most part servers will work the same as any Half-Life game or mod. There is one caveat involving the [network data system](../features/network-data-system.md) when using a FastDL server.

The Unified SDK generates a new `networkdata/data.json` file when it loads a map. When not using FastDL the file is immediately downloaded from the server.

When using FastDL the file server should include a dummy file containing only this:
```cpp
{}
```

This is an empty JSON file that will be downloaded and subsequently deleted by the client before downloading the generated file from the game server directly.

If this file is not present on the file server the game will still download the file from the game server but it will print an error message in the console which may confuse players.
