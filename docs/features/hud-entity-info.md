# Hud Entity Info

The hud has a new element called entity info.

This element draws entity information on the hud:

<a href="https://i.imgur.com/2FjKVmj.png">
<img src="https://i.imgur.com/2FjKVmj.png" width="800">
</a>

Any entity that is solid and can be hit by a traceline will show this information.

This hud element is drawn if the `sv_entityinfo_enabled` cvar is set to `1`. The default value for this cvar is `0`.

The `sv_entityinfo_eager` cvar controls whether this information stays on-screen after looking away. When set to `1`, the information immediately disappears, when set to `0` it stays for a total of `2` seconds (configurable with the `EntityInfoDrawTime` constant). The default value for this cvar is `1`.

By default the game will update the info every `0.2` seconds. This is configurable with the `EntityInfoUpdateInterval` constant. A shorter interval will result in more network activity.

The color for entity info fields is dependent on the type of the entity. By default the color is white, unless the entity is a monster. Monsters allied to the player have fields drawn in blue, whereas monsters hostile to the player have fields drawn in red. These colors were chosen to be colorblind-friendly.
