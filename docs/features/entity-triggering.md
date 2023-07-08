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
