# the name of the target operating system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER gcc-11)
set(CMAKE_C_FLAGS -m32)
set(CMAKE_CXX_COMPILER g++-11)
set(CMAKE_CXX_FLAGS -m32)

set(VCPKG_TARGET_TRIPLET "x86-linux" CACHE INTERNAL "Triplet to use for Linux development. Do not modify.")

include(${CMAKE_CURRENT_LIST_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake)
