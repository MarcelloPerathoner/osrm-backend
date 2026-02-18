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
target_link_libraries(node_osrm osrm_utils osrm_routed)

mk_build_dir("OSRM_NODEJS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")
push_env("OSRM_NODEJS_BUILD_DIR" "${OSRM_NODEJS_BUILD_DIR}")
push_env("OSRM_NODEJS_BINARY_DIR" "${CMAKE_CURRENT_BINARY_DIR}")

# Installation
# as a prelude to building the nodejs package with gyp

set(OSRM_NODEJS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/build/nodejs")
push_env("OSRM_NODEJS_INSTALL_DIR" "${OSRM_NODEJS_INSTALL_DIR}")

list(APPEND TARGETS node_osrm)
list(APPEND TARGETS osrm_routed)
list(APPEND TARGETS osrm_utils)
foreach(binary components contract customize datastore extract partition routed)
  list(APPEND TARGETS "osrm-${binary}")
endforeach()

foreach(t "${TARGETS}")
  set_property(TARGET ${t} PROPERTY INSTALL_RPATH_USE_LINK_PATH FALSE)
  if(APPLE)
    set_property(TARGET ${t} PROPERTY INSTALL_RPATH "@loader_path")
  endif()
  if(LINUX)
    set_property(TARGET ${t} PROPERTY INSTALL_RPATH "\$ORIGIN")
  endif()
endforeach()

# GET_RUNTIME_DEPENDENCIES() is too dumb to use the PATH when searching
# for DLLs. We have to spoon-feed it.
# string(REPLACE ":" ";" WIN_PATH "${CONAN_RUN_PATH}")
file(TO_NATIVE_PATH "$ENV{CONAN_RUN_PATH}" WIN_PATH)
message(STATUS "Win path is: ${WIN_PATH}")

message(STATUS "Conan run path is: ${CONAN_RUN_PATH}")  # this comes out with /
message(STATUS "Conan env run path is: $ENV{CONAN_RUN_PATH}")  # this comes out with \

install(
  TARGETS ${TARGETS}
  DESTINATION "${OSRM_NODEJS_INSTALL_DIR}/lib/binding_napi_v${NAPI_VERSION}"
  COMPONENT node_osrm
  RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES "api-ms-.*"
    POST_INCLUDE_REGEXES "conan2|boost|tbb|lua|bz2|zstd"
    POST_EXCLUDE_REGEXES ".*"
    DIRECTORIES ${CONAN_RUN_PATH}
)

install(
  FILES "index.js"
  DESTINATION "${OSRM_NODEJS_INSTALL_DIR}/lib"
  COMPONENT node_osrm
)

install(
  FILES "${PROJECT_SOURCE_DIR}/package.json"
  DESTINATION "${OSRM_NODEJS_INSTALL_DIR}"
  COMPONENT node_osrm
)
