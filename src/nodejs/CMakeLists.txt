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

# CMAKE_INSTALL_PREFIX is not available at config time
set(OSRM_NODEJS_INSTALL_PREFIX "build/nodejs")
set(OSRM_NODEJS_INSTALL_DIR    "${PROJECT_SOURCE_DIR}/${OSRM_NODEJS_INSTALL_PREFIX}")
set(OSRM_NODEJS_INSTALL_LIBDIR "${OSRM_NODEJS_INSTALL_DIR}/lib/lib")
set(OSRM_NODEJS_INSTALL_BINDIR "${OSRM_NODEJS_INSTALL_DIR}/lib/binding_napi_v${NAPI_VERSION}")

push_env("OSRM_NODEJS_INSTALL_PREFIX" "${OSRM_NODEJS_INSTALL_PREFIX}")
push_env("OSRM_NODEJS_INSTALL_DIR"    "${OSRM_NODEJS_INSTALL_DIR}")
push_env("OSRM_NODEJS_INSTALL_LIBDIR" "${OSRM_NODEJS_INSTALL_LIBDIR}")
push_env("OSRM_NODEJS_INSTALL_BINDIR" "${OSRM_NODEJS_INSTALL_BINDIR}")

#
# Installation
# as a prelude to building the nodejs package with gyp
#
# This is meant to be installed with ... --component node_package --prefix build/nodejs
#
# Our layout:
#
# CMAKE_INSTALL_LIBDIR/..:    build/nodejs                      package.json
# CMAKE_INSTALL_LIBDIR:       build/nodejs/lib/                 index.js
# OSRM_NODEJS_INSTALL_LIBDIR: build/nodejs/lib/lib              *.so
# OSRM_NODEJS_INSTALL_BINDIR: build/nodejs/lib/binding_napi_v8  node_osrm.node osrm-datastore *.dll
#

install(
  FILES "${PROJECT_SOURCE_DIR}/package.json"
  DESTINATION "${OSRM_NODEJS_INSTALL_DIR}"
  COMPONENT node_package
  EXCLUDE_FROM_ALL
)

install(
  FILES "index.js"
  DESTINATION "${OSRM_NODEJS_INSTALL_DIR}/lib"
  COMPONENT node_package
  EXCLUDE_FROM_ALL
)

# this target is actually a "lib" so we must install it explicitly into the "wrong"
# directory
install(
  TARGETS node_osrm
  DESTINATION "${OSRM_NODEJS_INSTALL_BINDIR}"
  COMPONENT node_package
  EXCLUDE_FROM_ALL
)
# also we must fix the RPATH
# set_property(TARGET node_osrm PROPERTY INSTALL_RPATH "${RPATH_ORIGIN}/../lib")


install(
  TARGETS ${ALL_BINARIES} ${ALL_LIBRARIES}
  RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES ${PRE_EXCLUDE_REGEXES}
    POST_INCLUDE_REGEXES ${POST_INCLUDE_REGEXES}
    POST_EXCLUDE_REGEXES ${POST_EXCLUDE_REGEXES}
    # defined in conan_toolchain.cmake
    DIRECTORIES ${CONAN_RUNTIME_LIB_DIRS} ${SANITIZER_LIBDIRS}
  RUNTIME
    DESTINATION "${OSRM_NODEJS_INSTALL_BINDIR}"
    COMPONENT node_package
    EXCLUDE_FROM_ALL
  LIBRARY # only .so, no .dll no .a
    DESTINATION "${OSRM_NODEJS_INSTALL_LIBDIR}"
    COMPONENT node_package
    EXCLUDE_FROM_ALL
  # https://gitlab.kitware.com/cmake/cmake/-/issues/25122
  FRAMEWORK DESTINATION "${OSRM_NODEJS_INSTALL_BINDIR}"
    EXCLUDE_FROM_ALL
)
