# Only needed so spdlog's setup doesn't complain. Can probably remove later.
set(CMAKE_SYSTEM_PROCESSOR x86)

set(VCPKG_TARGET_TRIPLET "x86-windows" CACHE INTERNAL "Triplet to use for Windows development. Do not modify.")

include(${CMAKE_CURRENT_LIST_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake)
