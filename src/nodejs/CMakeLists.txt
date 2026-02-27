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

file(GLOB TARGETS  src/contractor/*.cpp src/osrm/contractor.cpp)

list(APPEND TARGETS node_osrm)
foreach(binary components contract customize datastore extract partition routed)
  list(APPEND TARGETS "osrm-${binary}")
endforeach()
foreach(library contract customize extract partition routed store update utils)
  list(APPEND TARGETS "osrm_${library}")
endforeach()

install(
  TARGETS ${TARGETS}
  DESTINATION "${OSRM_NODEJS_INSTALL_DIR}/lib/binding_napi_v${NAPI_VERSION}"
  COMPONENT node_osrm
  RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES
        [[api-ms-win-.*]]
        [[ext-ms-.*]]
        [[kernel32\.dll]]
        [[libc\.so\..*]]
        [[libgcc_s\.so\..*]]
        [[libm\.so\..*]]
        [[libstdc\+\+\.so\..*]]
    POST_INCLUDE_REGEXES
        [[libboost.*]] [[boost.*\.dll]]
        [[libtbb.*]]   [[tbb.*\.dll]]
        [[liblua.*]]   [[lua.*\.dll]]
        [[libbz2.*]]   [[bz2\.dll]]
        [[libzstd.*]]  [[zstd.*\.dll]]
        [[libz\..*]]   [[z.*\.dll]]
        [[libosrm.*]]  [[osrm.*\.dll]]
    POST_EXCLUDE_REGEXES
        [[.*/system32/.*\.dll]]
        [[^/lib.*]]
        [[^/usr/lib.*]]
    # defined in conan_toolchain.cmake
    DIRECTORIES "${CONAN_RUNTIME_LIB_DIRS} ${OSRM_BUILD_DIR}"
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
