# func_recharge

**Brush entity**

Brush entity that allows players to charge their HEV suit by +USEing it.

> <span style="background-color:darkseagreen; color: black">
> Note
></br>
> Only changes are listed in this documentation. Consult TWHL's page for more information.</span>

[TWHL:func_recharge](https://twhl.info/wiki/page/func_recharge)

## Keyvalues

* **total_juice** - Custom amount of juice in this charger. A value of **-1** enables unlimited juice. If not specified the amount specified in the skill configuration is used
* **recharge_delay** - Custom recharge delay. A value of **-1** disables recharging. If not specified the amount specified in the skill configuration is used
* **charge_per_use** - Amount of juice to discharge per interval. Default **1**. If set to **0** the charger will never discharge
* **fire_on_recharge** - Target to trigger when the charger recharges itself
* **fire_on_spawn** - Whether to fire the `fire_on_recharge` target when the charger initially spawns
* **fire_on_empty** - Target to trigger when the charger runs out of juice
