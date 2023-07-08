# Entity Classifications

The Half-Life Unified SDK replaces the hard-coded classification system used by NPCs with one that level designers can customize to their needs.

This allows the relationship between NPCs to be changed globally and on a per-NPC basis.

Classifications are comparable to factions. The `human_passive` classification (Black Mesa scientists) fears `human_military` (Human Grunts) and will run away from them. `human_military` dislikes `human_passive` and will attack them.

The default classification file is `cfg/default_entity_classes.json`. A custom one can be used by specifying it in the map configuration file.

Only one configuration file can be used.

The entity classifications system uses the logger named `ent.classify`.

## Relationship types

| Name | Behavior |
| --- | --- |
| Ally | Pals. Good alternative to None when applicable. |
| Fear | Will run |
| None | Disregard |
| Dislike | Will attack |
| Hate | Will attack this character instead of any visible DISLIKEd characters |
| Nemesis | A monster Will ALWAYS attack its nemesis, no matter what |


## Syntax

An entity classifications configuration file is an object mapping a classification name to an object containing the relationships that classification has to other classifications.

The default relationship to all classifications is `none`. The default for a classification can be changed by setting the `DefaultRelationshipToTarget` to the chosen type:
```jsonc
"insect": {
	"DefaultRelationshipToTarget": "fear",
	"Relationships": {
		"alien_military": "none",
		"insect": "none",
		"player_bioweapon": "none",
		"alien_bioweapon": "none",
		"race_x": "none"
	}
}
```

This allows a classification's relationship to all other classifications to be set to the same type without having to explicitly specify the relationship.

The classification `none` is always added and should not have any relationships specified because entities rely on it to be completely disregarded.

### Example

```jsonc
{
    "none": {},
    "machine": {
        "Relationships": {
            "player": "dislike",
            "human_passive": "dislike",
            "human_military": "none",
            "alien_military": "dislike",
            "alien_passive": "dislike",
            "alien_monster": "dislike",
            "alien_prey": "dislike",
            "alien_predator": "dislike",
            "player_ally": "dislike",
            "player_bioweapon": "dislike",
            "alien_bioweapon": "dislike",
            "human_military_ally": "dislike",
            "race_x": "dislike"
        }
    },
    "player": {
        "Relationships": {
            "machine": "dislike",
            "human_military": "dislike",
            "alien_military": "dislike",
            "alien_passive": "dislike",
            "alien_monster": "dislike",
            "alien_prey": "dislike",
            "alien_predator": "dislike",
            "player_bioweapon": "dislike",
            "alien_bioweapon": "dislike",
            "race_x": "dislike"
        }
    },
    "human_passive": {
        "Relationships": {
            "player": "ally",
            "human_passive": "ally",
            "human_military": "hate",
            "alien_military": "hate",
            "alien_monster": "hate",
            "alien_prey": "dislike",
            "alien_predator": "dislike",
            "player_ally": "ally",
            "human_military_ally": "dislike",
            "race_x": "hate"
        }
    }
}
```

## See Also

* [EntityClassifications section](game-configuration-system.md#entityclassifications)
* [classification keyvalue](../entityguide/keyvalues-shared.md#classification)
* [child_classification keyvalue](../entityguide/keyvalues-shared.md#child_classification)
* [is_player_ally keyvalue](../entityguide/keyvalues-shared.md#is_player_ally)
