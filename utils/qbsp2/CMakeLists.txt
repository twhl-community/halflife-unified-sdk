add_executable(qbsp2
	bsp5.h
	cull.cpp
	gldraw.cpp
	merge.cpp
	#nodraw.cpp
	outside.cpp
	portals.cpp
	qbsp.cpp
	solidbsp.cpp
	surfaces.cpp
	tjunc.cpp
	writebsp.cpp)

target_compile_definitions(qbsp2
	PRIVATE
		DOUBLEVEC_T
		_NOENUMQBOOL)

target_link_libraries(qbsp2
	PRIVATE
		hl_sdk_utils_shared
		hl_sdk_utils_common
		OpenGL::GL
		OpenGL::GLU
		SDL2)

create_source_groups(qbsp2)
