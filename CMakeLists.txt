cmake_minimum_required(VERSION 3.16)

project(HalfLifeMod
	VERSION 1.0
	DESCRIPTION "Half-Life CMake Mod Template"
	LANGUAGES CXX)

#Try to get the directory path from registry if possible
get_filename_component(STEAM_HALFLIFE_DIRECTORY "[HKEY_CURRENT_USER\\Software\\Valve\\Steam;ModInstallPath]" ABSOLUTE)

# Used to locate binaries so dependencies are fixed up correctly (e.g. vgui.so)
set(HALFLIFE_DIRECTORY "${STEAM_HALFLIFE_DIRECTORY}" CACHE PATH "Path to the Half-Life directory")

set(HLDS_DIRECTORY "" CACHE PATH "Path to the Half-Life Dedicated Server directory. Must be specified to automatically set up dedicated server debugging settings")

if ("${HALFLIFE_DIRECTORY}" STREQUAL "")
	message(FATAL_ERROR "You must specify the path to the Half-Life directory")
elseif (NOT EXISTS ${HALFLIFE_DIRECTORY})
	message(FATAL_ERROR "The given Half-Life directory path ${HALFLIFE_DIRECTORY} is not valid")
endif()

# Set up vgui library
add_library(vgui SHARED IMPORTED)

if (WIN32)
	set_target_properties(vgui PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/utils/vgui/lib/win32_vc6/vgui.lib")
else()
	set_target_properties(vgui PROPERTIES IMPORTED_LOCATION "${HALFLIFE_DIRECTORY}/vgui.so")
endif()

target_include_directories(vgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/utils/vgui/include)

# Set up SDL2 library. No include path since it's used directly by the client project
add_library(SDL2 SHARED IMPORTED)

if (WIN32)
	set_target_properties(SDL2 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/public/SDL2.lib")
else()
	set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION "${HALFLIFE_DIRECTORY}/libSDL2.so")
endif()


add_library(client SHARED)

set_target_properties(client PROPERTIES PREFIX "")

# Link statically with the runtime
set_target_properties(client PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(client PRIVATE
	dlls
	cl_dll
	public
	common
	pm_shared
	engine
	utils/vgui/include
	game_shared
	external)
	
target_compile_definitions(client PRIVATE
	_CRT_SECURE_NO_WARNINGS
	$<$<CONFIG:DEBUG>:_DEBUG>
	CLIENT_DLL
	CLIENT_WEAPONS
	HL_DLL
	# These are defined for OSX as well
	$<$<PLATFORM_ID:Linux, Darwin>:
		POSIX
		_POSIX
		LINUX
		_LINUX
		GNUC
	>
	$<$<PLATFORM_ID:Darwin>:
		OSX
		_OSX
	>)

# TODO: review Linux compiler flags
target_compile_options(client PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -fpermissive -fno-strict-aliasing -Wno-invalid-offsetof>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W3 /Zc:threadSafeInit->)
	
target_link_libraries(client PRIVATE
	vgui
	$<$<PLATFORM_ID:Windows>:wsock32>
	SDL2)

add_subdirectory(cl_dll)

# Add the def file as well
add_library(hl SHARED dlls/hl.def)

set_target_properties(hl PROPERTIES PREFIX "")

# Link statically with the runtime
set_target_properties(hl PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(hl PRIVATE
	dlls
	engine
	common
	pm_shared
	game_shared
	public)
	
target_compile_definitions(hl PRIVATE
	_CRT_SECURE_NO_WARNINGS
	$<$<CONFIG:DEBUG>:_DEBUG>
	QUIVER
	VOXEL
	QUAKE2
	VALVE_DLL
	CLIENT_WEAPONS
	$<$<NOT:$<PLATFORM_ID:Windows>>:_LINUX>
	$<$<PLATFORM_ID:Linux>:LINUX>
	$<$<PLATFORM_ID:Darwin>:OSX>)

# TODO: review Linux compiler flags
target_compile_options(hl PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -fpermissive -fno-strict-aliasing -Wno-invalid-offsetof>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W3 /Zc:threadSafeInit->)

add_subdirectory(dlls)

# Create filters
get_target_property(SERVER_SOURCES hl SOURCES)
get_target_property(CLIENT_SOURCES client SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SERVER_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CLIENT_SOURCES})

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT client)

if (WIN32)
	set(HL_EXECUTABLE "hl.exe")
	set(HLDS_EXECUTABLE "hlds.exe")
else()
	set(HL_EXECUTABLE "hl_linux")
	set(HLDS_EXECUTABLE "hlds_linux")
endif()

# Set debugging settings to work on the mod directory
# It is assumed that the install path points to the mod directory
get_filename_component(MOD_NAME ${CMAKE_INSTALL_PREFIX} NAME_WE)

set_target_properties(client PROPERTIES
	VS_DEBUGGER_COMMAND "${HALFLIFE_DIRECTORY}/${HL_EXECUTABLE}"
	VS_DEBUGGER_COMMAND_ARGUMENTS "-game ${MOD_NAME}"
	VS_DEBUGGER_WORKING_DIRECTORY "${HALFLIFE_DIRECTORY}")

# If HLDS_DIRECTORY is not specified it will leave a non-functional "default" setting
set_target_properties(hl PROPERTIES
	VS_DEBUGGER_COMMAND "${HLDS_DIRECTORY}/${HLDS_EXECUTABLE}"
	VS_DEBUGGER_COMMAND_ARGUMENTS "-game ${MOD_NAME}"
	VS_DEBUGGER_WORKING_DIRECTORY "${HLDS_DIRECTORY}")
	
# Set install paths to the user-defined mod directory
install(TARGETS client
	RUNTIME DESTINATION cl_dlls
	LIBRARY DESTINATION cl_dlls
)

if (WIN32)
	install(FILES $<TARGET_PDB_FILE:client> DESTINATION cl_dlls OPTIONAL)
endif()

install(TARGETS hl
	RUNTIME DESTINATION dlls
	LIBRARY DESTINATION dlls
)

if (WIN32)
	install(FILES $<TARGET_PDB_FILE:hl> DESTINATION dlls OPTIONAL)
endif()
