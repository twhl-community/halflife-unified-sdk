# Entity Triggering

The Half-Life Unified SDK augments the entity triggering system with some new features. It is now possible to use wildcards at the end of target names to trigger multiple entities whose names start with the same prefix.

It is also possible to target entities using target selectors.

The entity trigger system uses the logger named `ent.io`.

## Wildcards

Using a wildcard character `*` in a target keyvalue matches any entity whose name starts with the prefix in the keyvalue.

For example, if you have a `func_door` named `room1_door1` and an `env_sprite` named `room1_sprite1` and a `func_button` that targets `room1_*` it will trigger both the door and sprite.

Wildcards are only supported at the end of a target name and cannot be used when naming entities themselves.

## Target Selectors

It is possible to use target selectors in some cases. For example you can kill the activator of a trigger like this:
```
"killtarget" "!activator"
```

Unfortunately some entities don't pass activators along properly, like `trigger_relay` which passes itself instead.

Two target selectors are supported:
* `!activator`: The entity that started the current trigger execution
* `!caller`: The last entity in the trigger execution chain

This can be used to target the player that activated a chain of entities.

## USE_TYPE

any entity that can trigger targets does support some extra keyvalues

- [m_UseType](#m_usetype) <**integer**>
- m_UseValue <**float**>

#### m_UseType

| Value | Enum | Description |
|---|---|---|
| -1 | USE_UNSET | This is the value by default and will have no effect at all. |
| 0 | USE_OFF | |
| 1 | USE_ON | |
| 2 | USE_SET | Does use of [m_UseValue](#m_usevalue-use_set) |
| 3 | USE_TOGGLE | |
| 4 | USE_KILL | Remove the entity from the world |
| 5 | USE_SAME | Sends the same USE_TYPE that **this** entity receive |
| 6 | USE_OPPOSITE | Sends the opposite USE_TYPE that **this** entity receive, only works for USE_OFF and USE_ON, if anything else then USE_TOGGLE is sent |
| 7 | USE_TOUCH | Calls the Touch function, pActivator will be passed as pOther |
| 8 | USE_LOCK | Locks the entity. Does use of [m_UseValue](#m_usevalue-use_un-lock) |
| 9 | USE_UNLOCK | Un-Locks the entity. Does use of [m_UseValue](#m_usevalue-use_un-lock) |
| ? | USE_UNKNOWN | Anything else will be count as USE_UNKNOWN and in fact, work the same as USE_UNSET |


#### m_UseValue USE_SET

#### m_UseValue USE_UN-LOCK

| Value | Enum | Description |
|---|---|---|
| 1 | USE_VALUE_LOCK_USE | Un-Locks the Use function |
| 2 | USE_VALUE_LOCK_TOUCH | Un-Locks the Touch function |
| 4 | USE_VALUE_LOCK_MASTER | Un-Locks the entity just if as a multisource was used |

- These are Writted and Read as Bits, So you can send more than one.