# Server library
include(../shared/game_shared.cmake)

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
	../shared/utils
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
	
	entities/NPCs/basemonster.h
	entities/NPCs/combat.cpp
	entities/NPCs/defaultai.cpp
	entities/NPCs/defaultai.h
	entities/NPCs/flyingmonster.cpp
	entities/NPCs/flyingmonster.h	
	entities/NPCs/h_ai.cpp
	entities/NPCs/monstermaker.cpp
	entities/NPCs/monsters.cpp
	entities/NPCs/monsters.h
	entities/NPCs/monsterstate.cpp
	entities/NPCs/schedule.cpp
	entities/NPCs/schedule.h
	entities/NPCs/scripted.cpp
	entities/NPCs/scripted.h
	entities/NPCs/scriptevent.h
	entities/NPCs/squad.h
	entities/NPCs/squadmonster.cpp
	entities/NPCs/squadmonster.h
	entities/NPCs/talkmonster.cpp
	entities/NPCs/talkmonster.h
	
	entities/NPCs/aliens/aflock.cpp
	entities/NPCs/aliens/agrunt.cpp
	entities/NPCs/aliens/barnacle.cpp
	entities/NPCs/aliens/bigmomma.cpp
	entities/NPCs/aliens/bloater.cpp
	entities/NPCs/aliens/bullsquid.cpp
	entities/NPCs/aliens/controller.cpp
	entities/NPCs/aliens/gargantua.cpp
	entities/NPCs/aliens/headcrab.cpp
	entities/NPCs/aliens/hornet.cpp
	entities/NPCs/aliens/hornet.h
	entities/NPCs/aliens/houndeye.cpp
	entities/NPCs/aliens/ichthyosaur.cpp
	entities/NPCs/aliens/islave.cpp
	entities/NPCs/aliens/leech.cpp
	entities/NPCs/aliens/nihilanth.cpp
	entities/NPCs/aliens/tentacle.cpp
	
	entities/NPCs/blackmesa/barney.cpp
	entities/NPCs/blackmesa/scientist.cpp
	
	entities/NPCs/blackops/hassassin.cpp
	
	entities/NPCs/military/apache.cpp
	entities/NPCs/military/hgrunt.cpp
	entities/NPCs/military/osprey.cpp
	entities/NPCs/military/turret.cpp
	
	entities/NPCs/other/genericmonster.cpp
	entities/NPCs/other/gman.cpp
	entities/NPCs/other/rat.cpp
	entities/NPCs/other/roach.cpp
	
	entities/NPCs/zombies/zombie.cpp

	gamerules/gamerules.cpp
	gamerules/gamerules.h
	gamerules/multiplay_gamerules.cpp
	gamerules/singleplay_gamerules.cpp
	gamerules/teamplay_gamerules.cpp
	gamerules/teamplay_gamerules.h
	
	../../common/mathlib.cpp
	../../common/Platform.h
	../../common/PlatformHeaders.h
	../../common/Sequence.h

	../../engine/edict.h
	../../engine/eiface.h
	../../engine/progdefs.h

	../../public/FileSystem.h
	../../public/interface.cpp
	../../public/interface.h
	../../public/steam/steamtypes.h)

add_game_shared_sources(hl)

# Create filters
get_target_property(SERVER_SOURCES hl SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../.. FILES ${SERVER_SOURCES})
