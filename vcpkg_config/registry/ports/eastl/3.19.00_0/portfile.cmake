vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO electronicarts/EASTL
    REF 5eb9b1ec09faaf5965125a07f3907a3d9cd0e1e7 #v3.19.00
    SHA512 01263a5c32ac44ac0ae7cac0e5d52a36edd4ebbc0fa5c38e5fcd5239738e201c7ee20067fa85f51a21ccaf524486022b6ac069d0b93c8d2e4f4f06795e321497
    HEAD_REF master
    PATCHES 
        fix_cmake_install.patch
        Fix-error-C2338.patch
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/EASTLConfig.cmake.in" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DEASTL_BUILD_TESTS=OFF
        -DEASTL_BUILD_BENCHMARK=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/EASTL)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_copy_pdbs()

# Handle copyright
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(INSTALL "${SOURCE_PATH}/3RDPARTYLICENSES.TXT" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

# CommonCppFlags used by EAThread
file(INSTALL "${SOURCE_PATH}/scripts/CMake/CommonCppFlags.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
