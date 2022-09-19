vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO pboettch/json-schema-validator
    REF 5b3200f83979519a41ffd6a43e6ae1f2a8dff3df
    SHA512 cd57124b625e9af580664acec968646de44b1a547857f362290db252b1e0480bbf59cb0ec2b69aefb797d5f0519af65d95a6c977882e314557afbb6f2a611137
    HEAD_REF main
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        -DJSON_VALIDATOR_BUILD_TESTS=OFF
        -DJSON_VALIDATOR_BUILD_EXAMPLES=OFF
)

vcpkg_install_cmake()

set(PKG_NAME "nlohmann_json_schema_validator")
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/${PKG_NAME} TARGET_PATH share/${PKG_NAME})

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})

