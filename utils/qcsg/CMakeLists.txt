add_executable(qcsg
	brush.cpp
	csg.h
	gldraw.cpp
	hullfile.cpp
	map.cpp
	qcsg.cpp
	textures.cpp)

target_compile_definitions(qcsg
	PRIVATE
		DOUBLEVEC_T)

target_link_libraries(qcsg
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common
		OpenGL::GL
		OpenGL::GLU
		SDL2)

create_source_groups(qcsg)
