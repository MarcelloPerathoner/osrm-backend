push_env("OSRM_TEST_DATA_DIR" "${PROJECT_SOURCE_DIR}/test/data")

mk_build_dir("OSRM_UNIT_TESTS_BUILD_DIR" "${CMAKE_CURRENT_BINARY_DIR}")
push_env("OSRM_UNIT_TESTS_BUILD_DIR" "${OSRM_UNIT_TESTS_BUILD_DIR}")

set(TEST_UPDATER_DATA_DIR "${PROJECT_SOURCE_DIR}/unit_tests/updater")
set(TEST_FIXTURES_DIR "${PROJECT_SOURCE_DIR}/unit_tests/fixtures")

file(GLOB EngineTestsSources
    engine_tests.cpp
    engine/*.cpp)

file(GLOB ContractorTestsSources
    contractor_tests.cpp
    contractor/*.cpp)

file(GLOB ExtractorTestsSources
    extractor_tests.cpp
    extractor/*.cpp)

file(GLOB PartitionTestsSources
    partitioner_tests.cpp
    partitioner/*.cpp)

file(GLOB CustomizerTestsSources
    customizer_tests.cpp
    customizer/*.cpp)

file(GLOB UpdaterTestsSources
    updater_tests.cpp
    updater/*.cpp)

file(GLOB StorageTestsSources
    storage_tests.cpp
    storage/*.cpp)

file(GLOB LibraryTestsSources
    library_tests.cpp
    library/*.cpp)
list(REMOVE_ITEM LibraryTestsSources
    ${CMAKE_CURRENT_SOURCE_DIR}/library/contract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/customize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/extract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/library/partition.cpp)

file(GLOB LibraryExtractTestsSources
    library_tests.cpp
    library/extract.cpp)

file(GLOB LibraryContractTestsSources
    library_tests.cpp
    library/contract.cpp)

file(GLOB LibraryCustomizeTestsSources
    library_tests.cpp
    library/customize.cpp)

file(GLOB LibraryPartitionTestsSources
    library_tests.cpp
    library/partition.cpp)

file(GLOB ServerTestsSources
    server_tests.cpp
    server/*.cpp)

file(GLOB UtilTestsSources
    util_tests.cpp
    util/*.cpp)

# add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable(contractor-tests	 EXCLUDE_FROM_ALL ${ContractorTestsSources})
add_executable(customizer-tests	 EXCLUDE_FROM_ALL ${CustomizerTestsSources})
add_executable(engine-tests	     EXCLUDE_FROM_ALL	${EngineTestsSources})
add_executable(extractor-tests   EXCLUDE_FROM_ALL	${ExtractorTestsSources})
add_executable(partitioner-tests EXCLUDE_FROM_ALL	${PartitionTestsSources})
add_executable(server-tests	     EXCLUDE_FROM_ALL	${ServerTestsSources})
add_executable(storage-tests	   EXCLUDE_FROM_ALL ${StorageTestsSources})
add_executable(updater-tests	   EXCLUDE_FROM_ALL ${UpdaterTestsSources})
add_executable(util-tests	       EXCLUDE_FROM_ALL	${UtilTestsSources})

add_executable(library-contract-tests  EXCLUDE_FROM_ALL	${LibraryContractTestsSources})
add_executable(library-customize-tests EXCLUDE_FROM_ALL ${LibraryCustomizeTestsSources})
add_executable(library-extract-tests   EXCLUDE_FROM_ALL	${LibraryExtractTestsSources})
add_executable(library-partition-tests EXCLUDE_FROM_ALL ${LibraryPartitionTestsSources})
add_executable(library-tests	         EXCLUDE_FROM_ALL	${LibraryTestsSources})

foreach(t contractor customizer engine extractor partitioner server storage updater util)
  list(APPEND TESTS "${t}-tests")
endforeach()

foreach(t contract customize extract partition)
  list(APPEND TESTS "library-${t}-tests")
endforeach()

list(APPEND TESTS "library-tests")

add_dependencies(library-tests osrm-extract osrm-contract osrm-partition)

foreach(t ${TESTS})
  target_compile_definitions(${t} PRIVATE COMPILE_DEFINITIONS OSRM_TEST_DATA_DIR="${OSRM_TEST_DATA_DIR}")
  target_include_directories(${t} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(${t} osrm_utils ${BOOST_UNIT_TEST_LIBRARIES})
endforeach()

target_compile_definitions(extractor-tests PRIVATE COMPILE_DEFINITIONS OSRM_FIXTURES_DIR="${TEST_FIXTURES_DIR}")
target_compile_definitions(updater-tests   PRIVATE COMPILE_DEFINITIONS TEST_UPDATER_DATA_DIR="${TEST_UPDATER_DATA_DIR}")

target_link_libraries(contractor-tests        osrm_contract)
target_link_libraries(customizer-tests        osrm_customize)
target_link_libraries(engine-tests            osrm_routed)
target_link_libraries(extractor-tests         osrm_extract)
target_link_libraries(library-contract-tests  osrm_contract)
target_link_libraries(library-customize-tests osrm_customize)
target_link_libraries(library-extract-tests   osrm_extract)
target_link_libraries(library-partition-tests osrm_partition)
target_link_libraries(library-tests           osrm_routed)
target_link_libraries(partitioner-tests       osrm_partition)
target_link_libraries(server-tests            osrm_routed)

add_custom_target(tests DEPENDS ${TESTS})
add_custom_target(osrm_tests DEPENDS ${TESTS})

### run the tests ###

add_test(NAME fixture-monaco COMMAND make -C "${OSRM_TEST_DATA_DIR}" monaco)
set_tests_properties(fixture-monaco PROPERTIES FIXTURES_SETUP test-fixtures-setup)

foreach(t ${TESTS})
  add_test(NAME "${t}" COMMAND "${t}")
  set_tests_properties("${t}" PROPERTIES FIXTURES_REQUIRED test-fixtures-setup LABELS tests)
endforeach()
