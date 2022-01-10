include(../shared/game_shared.cmake)

if (UNIX)
	# Copy libraries provided by the game to the build directory so we can point the compiler and linker to the files.
	configure_file(../../../utils/vgui/lib/linux/vgui${CMAKE_SHARED_LIBRARY_SUFFIX} ${CMAKE_CURRENT_BINARY_DIR}/vgui${CMAKE_SHARED_LIBRARY_SUFFIX} COPYONLY)
	
	configure_file(../../../external/SDL2/lib/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX}.0 ${CMAKE_CURRENT_BINARY_DIR}/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX}.0 COPYONLY)
endif()

# Set up vgui library
add_library(vgui SHARED IMPORTED)

if (WIN32)
	set_target_properties(vgui PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/vgui/lib/win32_vc6/vgui.lib")
else()
	set_target_properties(vgui PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/vgui${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

target_include_directories(vgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../../utils/vgui/include)

# Set up SDL2 library
add_library(SDL2 SHARED IMPORTED)

if (WIN32)
	set_target_properties(SDL2 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/../../../external/SDL2/lib/SDL2.lib")
else()
	set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/libSDL2-2.0${CMAKE_SHARED_LIBRARY_SUFFIX}.0")
endif()

target_include_directories(SDL2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/SDL2/include)

# Client library
add_library(client SHARED)

set_common_properties(client)
add_json_dependencies(client)

target_include_directories(client PRIVATE
	../server
	../server/entities
	../server/entities/NPCs
	../server/gamerules
	../shared/entities
	../shared/entities/items/weapons
	${CMAKE_CURRENT_SOURCE_DIR}
	input
	rendering
	ui
	ui/hud
	ui/vgui
	ui/vgui/utils
	../../public
	../../common
	../shared
	../shared/player_movement
	../shared/utils
	../../engine
	../../../utils/vgui/include)
	
target_compile_definitions(client PRIVATE
	CLIENT_DLL
	HL_DLL)
	
target_link_libraries(client PRIVATE
	vgui
	$<$<PLATFORM_ID:Windows>:wsock32>
	SDL2)
	
if (UNIX)
	# Tell the linker to search for vgui.so and SDL2-2.0.so.0 in here
	target_link_directories(client PRIVATE
		${CMAKE_CURRENT_BINARY_DIR})
endif()

target_link_options(client PRIVATE
	$<$<PLATFORM_ID:Darwin>:
		-framework Carbon>)

target_sources(client PRIVATE
	CClientLibrary.cpp
	CClientLibrary.h
	cdll_int.cpp
	cl_dll.h
	cl_util.h
	com_weapons.cpp
	com_weapons.h
	demo.cpp
	demo.h
	entity.cpp
	events.cpp
	eventscripts.h
	ev_common.cpp
	ev_hldm.cpp
	ev_hldm.h
	Exports.h
	global_consts.h
	interpolation.cpp
	interpolation.h
	in_defs.h
	player_info.h
	tf_defs.h
	util.cpp
	view.cpp
	view.h
	
	hl/hl_baseentity.cpp
	hl/hl_events.cpp
	hl/hl_objects.cpp
	hl/hl_weapons.cpp

	input/camera.h
	input/input.cpp
	input/inputw32.cpp
	input/in_camera.cpp
	input/kbutton.h

	rendering/GameStudioModelRenderer.cpp
	rendering/GameStudioModelRenderer.h
	rendering/StudioModelRenderer.cpp
	rendering/StudioModelRenderer.h
	rendering/studio_util.cpp
	rendering/studio_util.h
	rendering/tri.cpp
	rendering/tri.h

	ui/voice_banmgr.cpp
	ui/voice_banmgr.h
	ui/voice_status.cpp
	ui/voice_status.h

	ui/hud/ammo.cpp
	ui/hud/ammo.h
	ui/hud/ammohistory.cpp
	ui/hud/ammohistory.h
	ui/hud/ammo_secondary.cpp
	ui/hud/battery.cpp
	ui/hud/death.cpp
	ui/hud/flashlight.cpp
	ui/hud/geiger.cpp
	ui/hud/health.cpp
	ui/hud/health.h
	ui/hud/hud.cpp
	ui/hud/hud.h
	ui/hud/hud_flagicons.cpp
	ui/hud/hud_msg.cpp
	ui/hud/hud_playerbrowse.cpp
	ui/hud/hud_redraw.cpp
	ui/hud/hud_spectator.cpp
	ui/hud/hud_spectator.h
	ui/hud/hud_update.cpp
	ui/hud/menu.cpp
	ui/hud/message.cpp
	ui/hud/saytext.cpp
	ui/hud/scoreboard.cpp
	ui/hud/statusbar.cpp
	ui/hud/status_icons.cpp
	ui/hud/text_message.cpp
	ui/hud/train.cpp
	
	ui/vgui/vgui_ClassMenu.cpp
	ui/vgui/vgui_CustomObjects.cpp
	ui/vgui/vgui_int.cpp
	ui/vgui/vgui_int.h
	ui/vgui/vgui_MOTDWindow.cpp
	ui/vgui/vgui_SchemeManager.cpp
	ui/vgui/vgui_SchemeManager.h
	ui/vgui/vgui_ScorePanel.cpp
	ui/vgui/vgui_ScorePanel.h
	ui/vgui/vgui_SpectatorPanel.cpp
	ui/vgui/vgui_SpectatorPanel.h
	ui/vgui/vgui_StatsMenuPanel.cpp
	ui/vgui/vgui_StatsMenuPanel.h
	ui/vgui/vgui_TeamFortressViewport.cpp
	ui/vgui/vgui_TeamFortressViewport.h
	ui/vgui/vgui_teammenu.cpp
	ui/vgui/utils/vgui_checkbutton2.cpp
	ui/vgui/utils/vgui_checkbutton2.h
	ui/vgui/utils/vgui_defaultinputsignal.h
	ui/vgui/utils/vgui_grid.cpp
	ui/vgui/utils/vgui_grid.h
	ui/vgui/utils/vgui_helpers.cpp
	ui/vgui/utils/vgui_helpers.h
	ui/vgui/utils/vgui_listbox.cpp
	ui/vgui/utils/vgui_listbox.h
	ui/vgui/utils/vgui_loadtga.cpp
	ui/vgui/utils/vgui_loadtga.h
	ui/vgui/utils/vgui_scrollbar2.cpp
	ui/vgui/utils/vgui_scrollbar2.h
	ui/vgui/utils/vgui_slider2.cpp
	ui/vgui/utils/vgui_slider2.h

	../server/cdll_dll.h
	../server/enginecallback.h
	../server/extdll.h
	../server/entities/cbase.h
	../server/entities/NPCs/basemonster.h
	
	../../common/beamdef.h
	../../common/cl_entity.h
	../../common/com_model.h
	../../common/common_types.h
	../../common/const.h
	../../common/con_nprint.h
	../../common/crc.h
	../../common/cvardef.h
	../../common/demo_api.h
	../../common/director_cmds.h
	../../common/dlight.h
	../../common/dll_state.h
	../../common/entity_state.h
	../../common/entity_types.h
	../../common/enums.h
	../../common/event_api.h
	../../common/event_args.h
	../../common/event_flags.h
	../../common/hltv.h
	../../common/in_buttons.h
	../../common/ivoicetweak.h
	../../common/mathlib.cpp
	../../common/mathlib.h
	../../common/netadr.h
	../../common/net_api.h
	../../common/parsemsg.cpp
	../../common/parsemsg.h
	../../common/particledef.h
	../../common/Platform.h
	../../common/pmtrace.h
	../../common/qfont.h
	../../common/ref_params.h
	../../common/r_efx.h
	../../common/r_studioint.h
	../../common/screenfade.h
	../../common/Sequence.h
	../../common/studio_event.h
	../../common/triangleapi.h
	../../common/usercmd.h
	../../common/weaponinfo.h
	../../common/winsani_in.h
	../../common/winsani_out.h

	../../engine/APIProxy.h
	../../engine/cdll_int.h
	../../engine/custom.h
	../../engine/customentity.h
	../../engine/edict.h
	../../engine/eiface.h

	../../public/FileSystem.h
	../../public/interface.cpp
	../../public/interface.h
	../../public/keydefs.h
	../../public/particleman.h
	../../public/pman_particlemem.h
	../../public/pman_triangleffect.h
	../../public/cl_dll/IGameClientExports.h
	../../public/steam/steamtypes.h
)

add_game_shared_sources(client)

# Create filters
get_target_property(CLIENT_SOURCES client SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}../../../.. FILES ${CLIENT_SOURCES})
