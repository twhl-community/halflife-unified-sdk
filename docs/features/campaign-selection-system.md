# Campaign Selection System

The Half-Life Unified SDK uses a campaign selection menu to start the various Half-Life campaigns and training courses.

Each campaign is defined by a JSON file in the `campaigns` directory. This directory is checked in all [search paths](https://developer.valvesoftware.com/wiki/IFileSystemV009), meaning it is possible to add campaigns by putting them in `modname_addon/campaigns` for instance.

Each campaign contains a name and optional description, campaign and training map names. If either map name isn't provided the option to select it will be disabled. A campaign must provide at least one of these.

The campaigns are sorted based on the alphabetic order of the filename. A filename called `001HalfLife.json` will be sorted before `002OpposingForce.json`.

The campaign selection system uses the logger named `ui.campaign`.

## Example

```jsonc
{
	"Label": "Half-Life",
	"Description": "The Half-Life campaign.\n\nRelease date: 1998.",
	"CampaignMap": "c0a0",
	"TrainingMap": "t0a0"
}
```
