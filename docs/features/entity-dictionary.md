# Entity Dictionary

The Half-Life Unified SDK uses a dictionary to store the set of entities that exist in a mod. This dictionary can be used to create instances of entities without having to go through the engine to do so.

Headers:
* `game/shared/entities/EntityDictionary.h`

## Registering an entity

Registering an entity is done the same way as in the original Half-Life 1 SDK:
```cpp
LINK_ENTITY_TO_CLASS(info_player_start, CPointEntity);
```

The first name is the one used by level designers. It's the name you put in the mod's fgd file.

The second name is the name of the C++ class. This must be a class inheriting from `CBaseEntity` or one of its derived classes and must be a non-abstract class with a public, parameterless constructor.

## Creating an instance

When the engine creates an instance of an entity it does so using the function defined by the above `LINK_ENTITY_TO_CLASS` macro. 

This is done by the engine in two cases:
1. When a new map is starting and all of the entities in it are created
2. When a save game is being loaded and all entities are created up front prior to each entity being restored

To create an entity programmatically you can use the entity dictionary:
```cpp
CCrossbowBolt* pBolt = g_EntityDictionary->Create<CCrossbowBolt>("crossbow_bolt");
pBolt->Spawn();
```

This creates the entity named `crossbow_bolt` and returns it as a pointer to `CCrossbowBolt`.

The type must be compatible with that of the entity being created. In a debug build the dictionary checks to make sure this is the case and warns if it isn't.

Most code doesn't use the entity dictionary directly. Instead `CBaseEntity::Create` is used:
```cpp
CBaseEntity::Create(STRING(m_iszSpawnObject), VecBModelOrigin(this), pev->angles, this);
```

This function creates the entity, sets the origin, angles and owner and optionally calls the `Spawn` function which finishes setting up the entity.

`Spawn` must be called after the entity's keyvalues have been set.

## Destroying an entity

To destroy an entity simply call `UTIL_Remove` on it:
```cpp
UTIL_Remove(entityToDestroy);
```

This function can be called multiple times on the same entity.

Make sure to clear any pointers to entities that are being destroyed.

Prefer storing entity pointers in `EHANDLE` if you are storing it for any length of time.

## Additional dictionaries

It is possible to create entity dictionaries that store a set of entities that derive from a specific C++ class.

This can be done in the `RegisterEntityDescriptor` by adding some code:
```cpp
detail::RegisterEntityDescriptorToDictionaries(descriptor,
	EntityDictionaryLocator<CBaseEntity>::Get(),
	EntityDictionaryLocator<CBaseItem>::Get(),
	EntityDictionaryLocator<CBasePlayerWeapon>::Get());
```

For convenience global variables pointer to each dictionary are also created:
```cpp
inline EntityDictionary<CBaseEntity>* g_EntityDictionary = EntityDictionaryLocator<CBaseEntity>::Get();
inline EntityDictionary<CBaseItem>* g_ItemDictionary = EntityDictionaryLocator<CBaseItem>::Get();
inline EntityDictionary<CBasePlayerWeapon>* g_WeaponDictionary = EntityDictionaryLocator<CBasePlayerWeapon>::Get();
```

This can be used to restrict the creation of entities to specific types.

For example to create only items:
```cpp
auto entity = g_ItemDictionary->Create("monster_gman");

if (FNullEnt(entity))
{
	return;
}

DispatchSpawn(entity->edict());
```

The `Create` function returns pointers of the class type specified as the template parameter which helps to avoid bad runtime casts.

Classes will only register themselves to dictionaries that use a type that the class is derived from.

## Order of operations during entity creation

1. The constructor is called. Don't put any logic here as the entity is not ready for this yet.
2. The entity's `pev` variable is initialized.
3. `OnCreate` is called. This is where default values (e.g. health, model, etc) should be set.
4. `KeyValue` is called for each keyvalue set by the level designer. Keyvalues present in `entvars_t` are handled in `DispatchKeyValue`. Some keyvalues in `CBaseEntity` are also handled there by delegating to `RequiredKeyValue`.
4. `Spawn` is called by `DispatchSpawn` or by code programmatically creating the entity.
5. `Activate` is called during `ServerActivate` for all non-dormant entities. This happens when a new level is starting and when a save game is loaded to allow entities to find each-other. It is not called for entities created programmatically.
5. For entities that set a think function to run immediately, remaining setup logic is executed in the first few frames after startup.

## Order of operations during entity destruction

1. For entities destroyed by having `UTIL_Remove` called for them, `UpdateOnRemove` is called to allow cleanup code to run for entities removed programmatically.
2. `OnDestroy` is called. Cleanup code that should run regardless of removal method should be put here.
3. The destructor is called. The entity is no longer valid so no logic should be put here.

Note that entities are destroyed during level changes so care should be taken to test cleanup logic with save games.
