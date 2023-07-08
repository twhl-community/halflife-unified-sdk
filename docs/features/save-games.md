# Save Games

The Half-Life Unified SDK uses a redesigned save game system that allows modders to have greater control over the saving and restoring of game data.

The new system is decoupled from the engine's save data which allows new data types to be added. It is also possible to extend the system to save more complex types.

The save game system uses the logger named `saverestore`.
