# See https://cmake.org/cmake/help/latest/variable/CMAKE_MFC_FLAG.html
set(CMAKE_MFC_FLAG 2)

add_executable(serverctrl WIN32
	resource.h
	ServerCtrl.cpp
	ServerCtrl.h
	ServerCtrl.rc
	ServerCtrlDlg.cpp
	ServerCtrlDlg.h
	StdAfx.cpp
	StdAfx.h
	res/serverctrl.ico
	res/serverctrl.rc2)

target_precompile_headers(serverctrl
	PRIVATE
		StdAfx.h)

# TODO: replace winmm with std::chrono.
target_link_libraries(serverctrl
	PRIVATE
		hl_sdk_utils_shared
		winmm.lib)

create_source_groups(serverctrl)
