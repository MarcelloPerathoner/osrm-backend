cmake_minimum_required(VERSION 3.31)

message(STATUS "Building node_osrm")

set(NAPI_VERSION 8)
add_definitions(-DNAPI_VERSION=${NAPI_VERSION})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/nodejs")

include(NodeJS)
nodejs_init()

message(STATUS "Configuring node_osrm bindings for NodeJs ${NODEJS_VERSION}")

add_nodejs_module(node_osrm node_osrm.cpp)
set_target_properties(node_osrm PROPERTIES CXX_STANDARD 20)

# TODO: we disable clang-tidy for this target, because it causes errors in third-party NodeJs related headers
set_target_properties(node_osrm PROPERTIES CXX_CLANG_TIDY "")

execute_process(COMMAND node -p "require('node-addon-api').include.replaceAll('\"', '').trim()"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)

target_include_directories(node_osrm SYSTEM PRIVATE ${NODE_ADDON_API_DIR})

target_link_libraries(node_osrm osrm_utils osrm_routed)

disclose_build_dir("OSRM_NODEJS_BUILD_DIR" True)
