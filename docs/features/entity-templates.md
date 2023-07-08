# Entity Templates

The Half-Life Unified SDK provides a means of changing the default value of an entity's keyvalues using entity templates.

This can be used to globally change defaults such as the model used by an NPC or the amount of ammo in a weapon or ammo item.

Entity templates use the logger named `ent.template`.

## Syntax

An entity template is a JSON file containing an object containing keyvalue pairs. Values are strings and can be empty.

Templates are used by a map using the `EntityTemplates` section in a map configuration file.

It is recommended to place templates in the `cfg/entitytemplates` directory. Use subdirectories to group files together for map packs and for related actions.

### Example

```jsonc
{
	"default_ammo": "50" // Give MP5s a full magazine
}
```

## See Also

* [EntityTemplates section](game-configuration-system.md#entitytemplates)
