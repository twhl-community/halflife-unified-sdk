add_executable(mdlviewer
	mdlviewer.cpp
	mdlviewer.h
	studio_render.cpp
	studio_utils.cpp)

# TODO: fix conflicts between game and utils versions of mathlib
target_include_directories(mdlviewer
	PRIVATE
		../../src/common
		../../src/game/server)

target_link_libraries(mdlviewer
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common
		OpenGL::GL
		OpenGL::GLU
		SDL2)
