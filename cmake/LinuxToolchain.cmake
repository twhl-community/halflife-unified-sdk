set(VCPKG_TARGET_TRIPLET "x86-linux" CACHE INTERNAL "Triplet to use for Linux development. Do not modify.")

include (${CMAKE_CURRENT_LIST_DIR}/LinuxToolchainConfig.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake)
