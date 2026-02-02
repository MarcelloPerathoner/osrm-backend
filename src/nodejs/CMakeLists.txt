cmake_minimum_required(VERSION 3.18)

message(STATUS "Building node_osrm")

set(NAPI_VERSION 8)
add_definitions(-DNAPI_VERSION=${NAPI_VERSION})

set(BINDING_DIR "${PROJECT_SOURCE_DIR}/lib/binding_napi_v8")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/src/nodejs")
# list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/node_modules/node-cmake")

include(NodeJS)
nodejs_init()

message(STATUS "Configuring node_osrm bindings for NodeJs ${NODEJS_VERSION}")


add_nodejs_module(node_osrm node_osrm.cpp)
set_target_properties(node_osrm PROPERTIES CXX_STANDARD 20)
# TODO: we disable clang-tidy for this target, because it causes errors in third-party NodeJs related headers
set_target_properties(node_osrm PROPERTIES CXX_CLANG_TIDY "")
# TODO: we turn off some warnings for this target, because it causes errors in third-party NodeJs related headers
# target_no_warning(node_osrm suggest-destructor-override)
# target_no_warning(node_osrm suggest-override)

# https://github.com/cjntaylor/node-cmake/issues/53#issuecomment-842357457
execute_process(COMMAND node -p "require('node-addon-api').include" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE NODE_ADDON_API_DIR)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(node_osrm SYSTEM PRIVATE ${NODE_ADDON_API_DIR})

target_link_libraries(node_osrm osrm_utils osrm_routed)

# node_osrm artifacts in ${BINDING_DIR} to depend targets on
set(ARTIFACTS "")

# Create the binding directory at config time
# MSVC outputs a warning if we make a dependency on this and create it at run time:
# "MSB8065: Custom build for item binding_dir succeeded, but specified output
# binding_dir was not created.  Incremental compilation may not work correctly."
file(MAKE_DIRECTORY ${BINDING_DIR})

set(OSRM_BINARIES osrm-extract osrm-contract osrm-routed osrm-datastore osrm-components osrm-partition osrm-customize)
foreach(binary ${OSRM_BINARIES})
  set(t "${BINDING_DIR}/${binary}${CMAKE_EXECUTABLE_SUFFIX}")
  add_custom_command(OUTPUT "${t}"
                     COMMAND ${CMAKE_COMMAND} -E copy -t ${BINDING_DIR} $<TARGET_FILE:${binary}> $<TARGET_RUNTIME_DLLS:${binary}>
                     DEPENDS ${binary})
  list(APPEND ARTIFACTS "${t}")
endforeach(binary)

add_custom_command(OUTPUT ${BINDING_DIR}/node_osrm.node
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:node_osrm> ${BINDING_DIR}
                   DEPENDS node_osrm)
list(APPEND ARTIFACTS "${BINDING_DIR}/node_osrm.node")

message(STATUS "node_osrm artifacts will be copied to: ${BINDING_DIR}")
add_custom_target(copy_artifacts ALL DEPENDS ${ARTIFACTS})

if(NOT MSVC)
  add_custom_command(TARGET copy_artifacts POST_BUILD
    COMMAND python ${PROJECT_SOURCE_DIR}/scripts/ci/runtime_dependencies.py --grep \"boost|tbb|osrm\" --target ${BINDING_DIR} $<TARGET_FILE:node_osrm>
  )
endif()

disclose_build_dir("OSRM_NODEJS_BUILD_DIR")
