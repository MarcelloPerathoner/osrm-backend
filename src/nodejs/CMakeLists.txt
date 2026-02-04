cmake_minimum_required(VERSION 3.31)
cmake_policy(SET CMP0042 NEW)

set(CMAKE_CXX_STANDARD 20)
set(NAPI_VERSION 8)
add_definitions(-DNAPI_VERSION=${NAPI_VERSION})

if(ENABLE_CMAKE_JS)
  project (node_osrm)

  include_directories(${CMAKE_JS_INC})
  add_library(${PROJECT_NAME} SHARED "node_osrm.cpp" ${CMAKE_JS_SRC})
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
  target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
  if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    # Generate node.lib
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
  endif()
else()
  list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/nodejs")
  include(NodeJS)
  nodejs_init()

  add_nodejs_module(node_osrm node_osrm.cpp)
endif()

disclose_build_dir("OSRM_NODEJS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")

# TODO: we disable clang-tidy for this target, because it causes errors in third-party NodeJs related headers
set_target_properties(node_osrm PROPERTIES CXX_CLANG_TIDY "")
# TODO: we turn off some warnings for this target, because it causes errors in third-party NodeJs related headers
target_no_warning(node_osrm suggest-destructor-override)
target_no_warning(node_osrm suggest-override)

execute_process(COMMAND node -p "require('node-addon-api').include.replaceAll('\"', '')"
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(STRIP ${NODE_ADDON_API_DIR} NODE_ADDON_API_DIR)

message(STATUS "-I ${NODE_ADDON_API_DIR}")

target_include_directories(node_osrm PRIVATE "${NODE_ADDON_API_DIR}")
target_link_libraries(node_osrm osrm_utils osrm_routed)
