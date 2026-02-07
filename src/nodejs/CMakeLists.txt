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

disclose_build_dir("OSRM_NODEJS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")

# Installation
# as a prelude to building the nodejs package with gyp

list(APPEND TARGETS node_osrm)
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
string(REPLACE ":" ";" WIN_LIBRARY_PATH "$ENV{CONAN_LIBRARY_PATH}")

install(
  TARGETS ${TARGETS}
  DESTINATION "${PROJECT_SOURCE_DIR}/build/nodejs/lib/binding_napi_v${NAPI_VERSION}"
  COMPONENT node_osrm
  RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES "api-ms-.*"
    POST_INCLUDE_REGEXES "conan2|boost|tbb|lua|bz2|zstd"
    POST_EXCLUDE_REGEXES ".*"
    DIRECTORIES ${WIN_LIBRARY_PATH}
)

install(
  FILES "index.js"
  DESTINATION "${PROJECT_SOURCE_DIR}/build/nodejs/lib/"
  COMPONENT node_osrm
)

install(
  FILES "${PROJECT_SOURCE_DIR}/package.json"
  DESTINATION "${PROJECT_SOURCE_DIR}/build/nodejs/"
  COMPONENT node_osrm
)
