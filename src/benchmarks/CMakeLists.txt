set(DATA_DIR "${PROJECT_SOURCE_DIR}/test/data")
set(DATASET "berlin")
set(GPS_TRACES "${DATA_DIR}/${DATASET}_gps_traces.csv.gz")

add_executable(alias-bench        EXCLUDE_FROM_ALL alias.cpp)
add_executable(bench	          EXCLUDE_FROM_ALL bench.cpp)
add_executable(json-render-bench  EXCLUDE_FROM_ALL json_render.cpp)
add_executable(match-bench        EXCLUDE_FROM_ALL match.cpp)
add_executable(packedvector-bench EXCLUDE_FROM_ALL packed_vector.cpp)
add_executable(route-bench        EXCLUDE_FROM_ALL route.cpp)
add_executable(rtree-bench        EXCLUDE_FROM_ALL static_rtree.cpp)

target_link_libraries(alias-bench        osrm_utils                            ${MAYBE_SHAPEFILE})
target_link_libraries(bench	             osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(json-render-bench	 osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(match-bench        osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(packedvector-bench osrm_utils                            ${MAYBE_SHAPEFILE})
target_link_libraries(route-bench        osrm_utils ENGINE ${ENGINE_LIBRARIES} ${MAYBE_SHAPEFILE})
target_link_libraries(rtree-bench        osrm_utils                            ${MAYBE_SHAPEFILE})

target_include_directories(rtree-bench PUBLIC ${PROJECT_SOURCE_DIR}/unit_tests)

set (TESTS alias-bench bench json-render-bench match-bench packedvector-bench route-bench rtree-bench)

# build the benchmarks
add_custom_target(benchmarks DEPENDS ${TESTS})

### test the benchmarks ###

enable_testing()

# fixture for test matrix

add_test(NAME test_build COMMAND make -C "${DATA_DIR}" monaco berlin)
set_tests_properties(test_build PROPERTIES FIXTURES_SETUP test_fixture)

# test matrix

foreach(ALGORITHM ch mld)
  foreach(BENCHMARK match-bench route-bench)
    set (NAME "${BENCHMARK}-${ALGORITHM}")
    add_test(NAME "${NAME}" COMMAND match-bench "${DATA_DIR}/${ALGORITHM}/monaco.osrm" ${ALGORITHM})
    set_tests_properties("${NAME}" PROPERTIES FIXTURES_REQUIRED test_fixture)
  endforeach()
endforeach()

add_test(NAME alias-bench COMMAND alias-bench)
add_test(NAME json-render-bench COMMAND json-render-bench "${DATA_DIR}/portugal_to_korea.json")
add_test(NAME packedvector-bench COMMAND packedvector-bench 10 100000)
add_test(NAME rtree-bench COMMAND rtree-bench
                "${DATA_DIR}/monaco.osrm.ramIndex"
                "${DATA_DIR}/monaco.osrm.fileIndex"
                "${DATA_DIR}/monaco.osrm.nbg_nodes")

set_tests_properties(alias-bench        PROPERTIES FIXTURES_REQUIRED test_fixture)
set_tests_properties(json-render-bench  PROPERTIES FIXTURES_REQUIRED test_fixture)
set_tests_properties(packedvector-bench PROPERTIES FIXTURES_REQUIRED test_fixture)
set_tests_properties(rtree-bench        PROPERTIES FIXTURES_REQUIRED test_fixture)

foreach(ALGORITHM ch mld)
    foreach(BENCHMARK nearest table trip route match)
        set (NAME "random-${BENCHMARK}-${ALGORITHM}")
        add_test(
            NAME "${NAME}"
            COMMAND
                "${CMAKE_CURRENT_BINARY_DIR}/bench"
                "${DATA_DIR}/${ALGORITHM}/${DATASET}.osrm"
                "${ALGORITHM}"
                "${BENCHMARK}"
                5
                "${GPS_TRACES}")
        set_tests_properties("${NAME}" PROPERTIES FIXTURES_REQUIRED test_fixture)
        if(BUILD_NODE_PACKAGE)
            set (NAME "node-${BENCHMARK}-${ALGORITHM}")
            add_test(
                NAME "${NAME}"
                COMMAND
                    node "${PROJECT_SOURCE_DIR}/scripts/ci/bench.js"
                    "$ENV{OSRM_NODEJS_INSTALL_DIR}/lib/index.js"
                    "${DATA_DIR}/${ALGORITHM}/${DATASET}.osrm"
                    "${ALGORITHM}"
                    "${BENCHMARK}"
                    5
                    "${GPS_TRACES}")
            set_tests_properties("${NAME}" PROPERTIES FIXTURES_REQUIRED test_fixture)
        endif()
    endforeach()
endforeach()

disclose_build_dir("OSRM_BENCHMARKS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")
