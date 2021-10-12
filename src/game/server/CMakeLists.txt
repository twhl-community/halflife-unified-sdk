# Server library
# Add the def file as well
add_library(hl SHARED $<$<PLATFORM_ID:Windows>:hl.def>)

set_common_properties(hl)

target_include_directories(hl PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	entities
	entities/items
	../shared/entities
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
	entities/blowercannon.cpp
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
	entities/func_tank_of.cpp
	entities/h_cycler.cpp
	entities/lights.cpp
	entities/maprules.cpp
	entities/maprules.h
	entities/monsterevent.h
	entities/mortar.cpp
	entities/observer.cpp
	entities/op4mortar.cpp
	entities/pathcorner.cpp
	entities/plats.cpp
	entities/player.cpp
	entities/player.h
	entities/player_sethudcolor.cpp
	entities/sound.cpp
	entities/soundent.cpp
	entities/soundent.h
	entities/subs.cpp
	entities/trains.h
	entities/triggers.cpp
	entities/world.cpp
	entities/world.h
	entities/xen.cpp
	
	entities/ctf/CHUDIconTrigger.cpp
	entities/ctf/CHUDIconTrigger.h
	entities/ctf/CItemAcceleratorCTF.cpp
	entities/ctf/CItemAcceleratorCTF.h
	entities/ctf/CItemBackpackCTF.cpp
	entities/ctf/CItemBackpackCTF.h
	entities/ctf/CItemCTF.cpp
	entities/ctf/CItemCTF.h
	entities/ctf/CItemLongJumpCTF.cpp
	entities/ctf/CItemLongJumpCTF.h
	entities/ctf/CItemPortableHEVCTF.cpp
	entities/ctf/CItemPortableHEVCTF.h
	entities/ctf/CItemRegenerationCTF.cpp
	entities/ctf/CItemRegenerationCTF.h
	entities/ctf/CItemSpawnCTF.cpp
	entities/ctf/CItemSpawnCTF.h
	entities/ctf/CTFDefs.h
	entities/ctf/CTFDetect.cpp
	entities/ctf/CTFDetect.h
	entities/ctf/CTFGoal.cpp
	entities/ctf/CTFGoal.h
	entities/ctf/CTFGoalBase.cpp
	entities/ctf/CTFGoalBase.h
	entities/ctf/CTFGoalFlag.cpp
	entities/ctf/CTFGoalFlag.h
	entities/ctf/CTFSpawn.cpp
	entities/ctf/CTFSpawn.h
	
	entities/items/airtank.cpp
	entities/items/ggrenade.cpp
	entities/items/healthkit.cpp
	entities/items/h_battery.cpp
	entities/items/item_generic.cpp
	entities/items/items.cpp
	entities/items/items.h
	entities/items/nuclearbomb.cpp
	entities/items/weapons/CDisplacerBall.cpp
	entities/items/weapons/CDisplacerBall.h
	entities/items/weapons/CEagleLaser.cpp
	entities/items/weapons/CEagleLaser.h
	entities/items/weapons/CGrappleTip.cpp
	entities/items/weapons/CGrappleTip.h
	entities/items/weapons/CShockBeam.cpp
	entities/items/weapons/CShockBeam.h
	entities/items/weapons/CSpore.cpp
	entities/items/weapons/CSpore.h
	entities/items/weapons/weapons.cpp
	
	entities/rope/CElectrifiedWire.cpp
	entities/rope/CElectrifiedWire.h
	entities/rope/CRope.cpp
	entities/rope/CRope.h
	entities/rope/CRopeSample.cpp
	entities/rope/CRopeSample.h
	entities/rope/CRopeSegment.cpp
	entities/rope/CRopeSegment.h
	
	../shared/entities/ehandle.cpp
	../shared/entities/ehandle.h
	../shared/entities/items/weapons/CDisplacer.cpp
	../shared/entities/items/weapons/CDisplacer.h
	../shared/entities/items/weapons/CEagle.cpp
	../shared/entities/items/weapons/CEagle.h
	../shared/entities/items/weapons/CGrapple.cpp
	../shared/entities/items/weapons/CGrapple.h
	../shared/entities/items/weapons/CKnife.cpp
	../shared/entities/items/weapons/CKnife.h
	../shared/entities/items/weapons/CM249.cpp
	../shared/entities/items/weapons/CM249.h
	../shared/entities/items/weapons/CPenguin.cpp
	../shared/entities/items/weapons/CPenguin.h
	../shared/entities/items/weapons/CPipewrench.cpp
	../shared/entities/items/weapons/CPipewrench.h
	../shared/entities/items/weapons/crossbow.cpp
	../shared/entities/items/weapons/crowbar.cpp
	../shared/entities/items/weapons/CShockRifle.cpp
	../shared/entities/items/weapons/CShockRifle.h
	../shared/entities/items/weapons/CSniperRifle.cpp
	../shared/entities/items/weapons/CSniperRifle.h
	../shared/entities/items/weapons/CSporeLauncher.cpp
	../shared/entities/items/weapons/CSporeLauncher.h
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
	entities/NPCs/aliens/gonome.cpp
	entities/NPCs/aliens/headcrab.cpp
	entities/NPCs/aliens/hornet.cpp
	entities/NPCs/aliens/hornet.h
	entities/NPCs/aliens/houndeye.cpp
	entities/NPCs/aliens/ichthyosaur.cpp
	entities/NPCs/aliens/islave.cpp
	entities/NPCs/aliens/leech.cpp
	entities/NPCs/aliens/nihilanth.cpp
	entities/NPCs/aliens/tentacle.cpp
	entities/NPCs/aliens/zombie.cpp
	entities/NPCs/aliens/zombie_barney.cpp
	entities/NPCs/aliens/zombie_soldier.cpp
	
	entities/NPCs/blackmesa/barney.cpp
	entities/NPCs/blackmesa/cleansuit_scientist.cpp
	entities/NPCs/blackmesa/loader.cpp
	entities/NPCs/blackmesa/otis.cpp
	entities/NPCs/blackmesa/rosenberg.cpp
	entities/NPCs/blackmesa/scientist.cpp
	entities/NPCs/blackmesa/scientist.h
	
	entities/NPCs/blackops/blkop_apache.cpp
	entities/NPCs/blackops/blkop_osprey.cpp
	entities/NPCs/blackops/hassassin.cpp
	entities/NPCs/blackops/male_assassin.cpp
	
	entities/NPCs/military/apache.cpp
	entities/NPCs/military/COFAllyMonster.cpp
	entities/NPCs/military/COFAllyMonster.h
	entities/NPCs/military/COFSquadTalkMonster.cpp
	entities/NPCs/military/COFSquadTalkMonster.h
	entities/NPCs/military/drill_sergeant.cpp
	entities/NPCs/military/hfgrunt.cpp
	entities/NPCs/military/hgrunt.cpp
	entities/NPCs/military/hgrunt_ally.cpp
	entities/NPCs/military/hgrunt_medic.cpp
	entities/NPCs/military/hgrunt_torch.cpp
	entities/NPCs/military/osprey.cpp
	entities/NPCs/military/recruit.cpp
	entities/NPCs/military/turret.cpp
	
	entities/NPCs/other/genericmonster.cpp
	entities/NPCs/other/gman.cpp
	entities/NPCs/other/penguin_grenade.cpp
	entities/NPCs/other/rat.cpp
	entities/NPCs/other/roach.cpp
	entities/NPCs/other/skeleton.cpp
	
	entities/NPCs/racex/baby_voltigore.cpp
	entities/NPCs/racex/geneworm.cpp
	entities/NPCs/racex/pitdrone.cpp
	entities/NPCs/racex/pitworm_up.cpp
	entities/NPCs/racex/shockroach.cpp
	entities/NPCs/racex/shocktrooper.cpp
	entities/NPCs/racex/voltigore.cpp
	
	gamerules/coopplay_gamerules.cpp
	gamerules/coopplay_gamerules.h
	gamerules/ctfplay_gamerules.cpp
	gamerules/ctfplay_gamerules.h
	gamerules/gamerules.cpp
	gamerules/gamerules.h
	gamerules/multiplay_gamerules.cpp
	gamerules/singleplay_gamerules.cpp
	gamerules/teamplay_gamerules.cpp
	gamerules/teamplay_gamerules.h
	
	../../common/mathlib.cpp
	../../common/Sequence.h

	../../engine/edict.h
	../../engine/eiface.h
	../../engine/progdefs.h

	../../public/steam/steamtypes.h

	../shared/bitvec.h
	../shared/palette.h
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
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/../.. FILES ${SERVER_SOURCES})
