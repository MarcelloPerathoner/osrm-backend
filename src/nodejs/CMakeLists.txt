cmake_minimum_required(VERSION 3.31)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_CXX_STANDARD 20)
set(NAPI_VERSION 8)
add_definitions(-DNAPI_VERSION=${NAPI_VERSION})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/nodejs")
include(NodeJS)
nodejs_init()

add_nodejs_module(node_osrm node_osrm.cpp)

# TODO: we disable clang-tidy for this target, because it causes errors in third-party NodeJs related headers
set_target_properties(node_osrm PROPERTIES CXX_CLANG_TIDY "")
# TODO: we turn off some warnings for this target, because it causes errors in third-party NodeJs related headers
target_no_warning(node_osrm suggest-destructor-override)

execute_process(COMMAND node -p "require('node-addon-api').include.replaceAll('\"', '')"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(STRIP ${NODE_ADDON_API_DIR} NODE_ADDON_API_DIR)

message(STATUS "-I ${NODE_ADDON_API_DIR}")

target_include_directories(node_osrm PRIVATE "${NODE_ADDON_API_DIR}")
target_link_libraries(node_osrm osrm_routed osrm_utils)

mk_build_dir("OSRM_NODEJS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")
push_env("OSRM_NODEJS_BUILD_DIR" "${OSRM_NODEJS_BUILD_DIR}")

# Installation
# as a prelude to building the nodejs package with gyp

set(OSRM_NODEJS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/build/nodejs")
push_env("OSRM_NODEJS_INSTALL_DIR" "${OSRM_NODEJS_INSTALL_DIR}")

### This is meant to be installed with --component node_package --prefix build/nodejs ###

install(
  FILES "${PROJECT_SOURCE_DIR}/package.json"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/.."
  COMPONENT node_package
  EXCLUDE_FROM_ALL
)

install(
  FILES "index.js"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  COMPONENT node_package
  EXCLUDE_FROM_ALL
)

install(
  TARGETS node_osrm
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/binding_napi_v${NAPI_VERSION}"
  COMPONENT node_package
  EXCLUDE_FROM_ALL
)
set_property(TARGET node_osrm PROPERTY INSTALL_RPATH "${RPATH_ORIGIN}/..")
