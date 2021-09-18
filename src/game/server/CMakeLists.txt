# Server library
# Add the def file as well
add_library(hl SHARED $<$<PLATFORM_ID:Windows>:hl.def>)

set_common_properties(hl)

target_include_directories(hl PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	entities
	entities/items
	../shared/entities/items/weapons
	entities/NPCs
	gamerules
	../../engine
	../../common
	../shared
	../shared/player_movement
	../../public)
	
target_compile_definitions(hl PRIVATE
	VALVE_DLL)

target_sources(hl PRIVATE
	activity.h
	activitymap.h
	cdll_dll.h
	client.cpp
	client.h
	decals.h
	enginecallback.h
	extdll.h
	game.cpp
	game.h
	globals.cpp
	h_export.cpp
	nodes.cpp
	nodes.h
	plane.cpp
	plane.h
	saverestore.h
	skill.cpp
	skill.h
	UserMessages.cpp
	UserMessages.h
	util.cpp
	util.h
	vector.h
	voice_gamemgr.cpp
	voice_gamemgr.h
	
	entities/animating.cpp
	entities/animation.cpp
	entities/animation.h
	entities/bmodels.cpp
	entities/buttons.cpp
	entities/cbase.cpp
	entities/cbase.h
	entities/doors.cpp
	entities/doors.h
	entities/effects.cpp
	entities/effects.h
	entities/explode.cpp
	entities/explode.h
	entities/func_break.cpp
	entities/func_break.h
	entities/func_tank.cpp
	entities/h_cycler.cpp
	entities/lights.cpp
	entities/maprules.cpp
	entities/maprules.h
	entities/monsterevent.h
	entities/observer.cpp
	
	entities/items/airtank.cpp
	entities/items/ggrenade.cpp
	entities/items/healthkit.cpp
	entities/items/h_battery.cpp
	entities/items/items.cpp
	entities/items/items.h
	entities/mortar.cpp
	entities/pathcorner.cpp
	entities/plats.cpp
	entities/player.cpp
	entities/player.h
	entities/sound.cpp
	entities/soundent.cpp
	entities/soundent.h
	entities/subs.cpp
	entities/trains.h
	entities/triggers.cpp
	entities/world.cpp
	entities/xen.cpp
	
	entities/items/weapons/weapons.cpp
	
	../shared/entities/items/weapons/crossbow.cpp
	../shared/entities/items/weapons/crowbar.cpp
	../shared/entities/items/weapons/egon.cpp
	../shared/entities/items/weapons/gauss.cpp
	../shared/entities/items/weapons/handgrenade.cpp
	../shared/entities/items/weapons/hornetgun.cpp
	../shared/entities/items/weapons/hl_wpn_glock.cpp
	../shared/entities/items/weapons/mp5.cpp
	../shared/entities/items/weapons/python.cpp
	../shared/entities/items/weapons/rpg.cpp
	../shared/entities/items/weapons/satchel.cpp
	../shared/entities/items/weapons/shotgun.cpp
	../shared/entities/items/weapons/squeakgrenade.cpp
	../shared/entities/items/weapons/tripmine.cpp
	../shared/entities/items/weapons/weapons_shared.cpp
	../shared/entities/items/weapons/weapons.h
	
	entities/NPCs/aflock.cpp
	entities/NPCs/agrunt.cpp
	entities/NPCs/apache.cpp
	entities/NPCs/barnacle.cpp
	entities/NPCs/barney.cpp
	entities/NPCs/basemonster.h
	entities/NPCs/bigmomma.cpp
	entities/NPCs/bloater.cpp
	entities/NPCs/bullsquid.cpp
	entities/NPCs/combat.cpp
	entities/NPCs/controller.cpp
	entities/NPCs/defaultai.cpp
	entities/NPCs/defaultai.h
	entities/NPCs/flyingmonster.cpp
	entities/NPCs/flyingmonster.h
	entities/NPCs/gargantua.cpp
	entities/NPCs/genericmonster.cpp
	entities/NPCs/gman.cpp
	entities/NPCs/hassassin.cpp
	entities/NPCs/headcrab.cpp
	entities/NPCs/hgrunt.cpp
	entities/NPCs/hornet.cpp
	entities/NPCs/hornet.h
	entities/NPCs/houndeye.cpp
	entities/NPCs/h_ai.cpp
	entities/NPCs/ichthyosaur.cpp
	entities/NPCs/islave.cpp
	entities/NPCs/leech.cpp
	entities/NPCs/monstermaker.cpp
	entities/NPCs/monsters.cpp
	entities/NPCs/monsters.h
	entities/NPCs/monsterstate.cpp
	entities/NPCs/nihilanth.cpp
	entities/NPCs/osprey.cpp
	entities/NPCs/rat.cpp
	entities/NPCs/roach.cpp
	entities/NPCs/schedule.cpp
	entities/NPCs/schedule.h
	entities/NPCs/scientist.cpp
	entities/NPCs/scripted.cpp
	entities/NPCs/scripted.h
	entities/NPCs/scriptevent.h
	entities/NPCs/squad.h
	entities/NPCs/squadmonster.cpp
	entities/NPCs/squadmonster.h
	entities/NPCs/talkmonster.cpp
	entities/NPCs/talkmonster.h
	entities/NPCs/tentacle.cpp
	entities/NPCs/turret.cpp
	entities/NPCs/zombie.cpp
	
	gamerules/gamerules.cpp
	gamerules/gamerules.h
	gamerules/multiplay_gamerules.cpp
	gamerules/singleplay_gamerules.cpp
	gamerules/teamplay_gamerules.cpp
	gamerules/teamplay_gamerules.h
	
	../../common/mathlib.cpp
	../shared/voice_common.h
	../shared/player_movement/pm_debug.cpp
	../shared/player_movement/pm_debug.h
	../shared/player_movement/pm_defs.h
	../shared/player_movement/pm_info.h
	../shared/player_movement/pm_materials.h
	../shared/player_movement/pm_math.cpp
	../shared/player_movement/pm_movevars.h
	../shared/player_movement/pm_shared.cpp
	../shared/player_movement/pm_shared.h)

# Create filters
get_target_property(SERVER_SOURCES hl SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../../.. FILES ${SERVER_SOURCES})
