vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO pboettch/json-schema-validator
    REF 1063c9adbafc25f5a14bae15c3babdb039de86c6
    SHA512 c60f08c449af7faceac830d5f497d5c9328a3363205123ebd91ed86f20937c7e0bd31c1d7165166cd74ce92be0d2cf7dc0a0f8faeb447dd080682bb8694897da
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

