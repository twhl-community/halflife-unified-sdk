cmake_minimum_required(VERSION 3.16)

project(HalfLifeMod
	VERSION 1.0
	DESCRIPTION "Half-Life CMake Mod Template"
	LANGUAGES CXX)

if (WIN32)
	#Try to get the directory path from registry if possible
	get_filename_component(STEAM_HALFLIFE_DIRECTORY "[HKEY_CURRENT_USER\\Software\\Valve\\Steam;ModInstallPath]" ABSOLUTE)
endif()

# Used to set up debugging settings on Windows
set(HALFLIFE_DIRECTORY "${STEAM_HALFLIFE_DIRECTORY}" CACHE PATH "Path to the Half-Life directory")

if (MSVC)
	set(HLDS_DIRECTORY "" CACHE PATH "Path to the Half-Life Dedicated Server directory. Must be specified to automatically set up dedicated server debugging settings")
endif()

if (NOT "${HALFLIFE_DIRECTORY}" STREQUAL "" AND NOT EXISTS ${HALFLIFE_DIRECTORY})
	message(FATAL_ERROR "The given Half-Life directory path ${HALFLIFE_DIRECTORY} is not valid")
endif()

# Find dependencies
find_package(Threads REQUIRED)

# Copy libraries provided by the game to the build directory so relative link paths work at build time (matches the SDK's way of doing things)
configure_file(linux/release/vgui.so vgui.so COPYONLY)
configure_file(linux/libSDL2-2.0.so.0 libSDL2-2.0.so.0 COPYONLY)

# Set up vgui library
add_library(vgui SHARED IMPORTED)

if (WIN32)
	set_target_properties(vgui PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/utils/vgui/lib/win32_vc6/vgui.lib")
else()
	set_target_properties(vgui PROPERTIES IMPORTED_LOCATION "vgui.so")
endif()

target_include_directories(vgui INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/utils/vgui/include)

# Set up SDL2 library. No include path since it's used directly by the client project
add_library(SDL2 SHARED IMPORTED)

if (WIN32)
	set_target_properties(SDL2 PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/public/SDL2.lib")
else()
	set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION "libSDL2-2.0.so.0")
endif()

# Sets properties common to all projects
function(set_common_properties TARGET_NAME)
	set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")

	# Link statically with the runtime
	set_target_properties(${TARGET_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		
	target_compile_definitions(${TARGET_NAME} PRIVATE
		_CRT_SECURE_NO_WARNINGS
		$<$<CONFIG:DEBUG>:_DEBUG>
		CLIENT_WEAPONS
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
		
	target_link_libraries(${TARGET_NAME} PRIVATE
		Threads::Threads
		$<$<PLATFORM_ID:Linux,Darwin>:dl>)

	# TODO: review Linux compiler flags
	target_compile_options(${TARGET_NAME} PRIVATE
		# force 387 for FP math so the precision between win32 and linux and osx match
		# Note: the pentium-m arch setting is not used for AMD systems in the original makefile
		# Since the arch settings used are i686 this means including the setting ensures original settings are used,
		# but it could cause problems for AMD targets
		$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
			-m32 -fpermissive -fno-strict-aliasing -Wno-invalid-offsetof -gdwarf-2 -g2 -march=pentium-m -mfpmath=387>
			
		$<$<CXX_COMPILER_ID:MSVC>:
			/W3 /Zc:threadSafeInit->
			
		# These are all from the original Makefile
		# They have not been tested with CMake since there is no Mac system available to test on, so this might not work
		$<$<PLATFORM_ID:Darwin>:
			-fvisibility=hidden -Qunused-arguments -mmacosx-version-min=10.5 -fasm-blocks -arch i386 -march=prescott -momit-leaf-frame-pointer -mtune=core2>)

	target_link_options(${TARGET_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
			-m32 -gdwarf-2 -g2>
		$<$<PLATFORM_ID:Linux>:
			-Wl,-Map,${TARGET_NAME}_map.txt>)
endfunction()

# Add client library
add_library(client SHARED)

set_common_properties(client)

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
	CLIENT_DLL
	HL_DLL)
	
target_link_libraries(client PRIVATE
	vgui
	$<$<PLATFORM_ID:Windows>:wsock32>
	SDL2)

target_link_options(client PRIVATE
	$<$<PLATFORM_ID:Darwin>:
		-framework Carbon>)

add_subdirectory(cl_dll)

# Add server library
# Add the def file as well
add_library(hl SHARED dlls/hl.def)

set_common_properties(hl)

target_include_directories(hl PRIVATE
	dlls
	engine
	common
	pm_shared
	game_shared
	public)
	
target_compile_definitions(hl PRIVATE
	QUIVER
	VOXEL
	QUAKE2
	VALVE_DLL)

add_subdirectory(dlls)

# Create filters
get_target_property(SERVER_SOURCES hl SOURCES)
get_target_property(CLIENT_SOURCES client SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SERVER_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CLIENT_SOURCES})

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT client)

if (MSVC AND NOT "${HALFLIFE_DIRECTORY}" STREQUAL "")
	set(HL_EXECUTABLE "hl.exe")
	set(HLDS_EXECUTABLE "hlds.exe")

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
endif()
	
# Set install paths to the user-defined mod directory
install(TARGETS client
	RUNTIME DESTINATION cl_dlls
	LIBRARY DESTINATION cl_dlls
)

install(TARGETS hl
	RUNTIME DESTINATION dlls
	LIBRARY DESTINATION dlls
)

if (WIN32)
	install(FILES $<TARGET_PDB_FILE:client> DESTINATION cl_dlls OPTIONAL)
	install(FILES $<TARGET_PDB_FILE:hl> DESTINATION dlls OPTIONAL)
endif()
