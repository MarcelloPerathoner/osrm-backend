# instrumentation like coverage, fuzzing, sanitizers

set(MAYBE_COVERAGE_LIBRARIES "")
if(ENABLE_COVERAGE)
  message(STATUS "Enabling coverage")
  set(MAYBE_COVERAGE_LIBRARIES "-lgcov")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ftest-coverage -fprofile-arcs")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-instr-generate -fcoverage-mapping")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fcoverage-mapping")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fcoverage-mapping")
  endif()
endif()

if(ENABLE_FUZZING)
  # Requires libosrm being built with sanitizers; make configurable and default to ubsan
  set(FUZZ_SANITIZER "undefined" CACHE STRING "Sanitizer to be used for Fuzz testing")
  set_property(CACHE FUZZ_SANITIZER PROPERTY STRINGS "undefined" "integer" "address" "memory" "thread" "leak")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-coverage=edge,indirect-calls,8bit-counters")
  set(ENABLE_ASAN ON)

  message(STATUS "Using -fsanitize=${FUZZ_SANITIZER} for Fuzz testing")

  add_subdirectory(fuzz)
endif()

macro(print_file_name)
  # We cannot just find_library() here because the user may have both gcc and clang
  # libraries installed. We must match the library to the compiler.
  foreach(lib ${ARGN})
    execute_process(
      COMMAND "${CMAKE_CXX_COMPILER}" "-print-file-name=${lib}"
      OUTPUT_VARIABLE TMP
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT ("${TMP}" STREQUAL "${lib}"))
      message(STATUS "Found library: ${lib} => ${TMP}")
      list(APPEND SANITIZER_LD_PRELOAD "${TMP}")
      cmake_path(GET TMP PARENT_PATH TMP)
      list(APPEND SANITIZER_LIBDIRS "${TMP}")
    endif()
  endforeach()
endmacro()

if(ENABLE_ASAN)
  # See: https://clang.llvm.org/docs/AddressSanitizer.html
  # See: https://clang.llvm.org/docs/LeakSanitizer.html
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND SANITIZER_CXX_FLAGS -fsanitize=address -fsanitize-address-use-after-scope -shared-libasan -O1 -U_FORTIFY_SOURCE)
    list(APPEND SANITIZER_LINKER_FLAGS -fsanitize=address)
    print_file_name("libclang_rt.asan-x86_64.so" "libclang_rt.asan_osx_dynamic.dylib")
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    list(APPEND SANITIZER_CXX_FLAGS -fsanitize=address -fsanitize-address-use-after-scope)
    list(APPEND SANITIZER_CXX_FLAGS --param max-gcse-memory=2048000 -U_FORTIFY_SOURCE)
    list(APPEND SANITIZER_LINKER_FLAGS -fsanitize=address)
    no_warning(uninitialized)
    no_warning(maybe-uninitialized)
    # no_warning(disabled-optimization)
    print_file_name("libasan.so")
  endif()
  if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # This does not work yet. TODO see:
    # https://mmomtchev.medium.com/debugging-random-memory-corruption-with-asan-in-a-node-js-addon-on-windows-with-msvc-6246af0c22c7
    list(APPEND SANITIZER_CXX_FLAGS /fsanitize=address)
  endif()
endif()

if(ENABLE_TSAN)
  list(APPEND SANITIZER_CXX_FLAGS    -fsanitize=thread)
  list(APPEND SANITIZER_LINKER_FLAGS -fsanitize=thread)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    print_file_name("libtsan.so")
  else()
    print_file_name("libclang_rt.tsan-x86_64.so")
  endif()
endif()

if(ENABLE_UBSAN)
  list(APPEND SANITIZER_CXX_FLAGS    -fsanitize=undefined)
  list(APPEND SANITIZER_LINKER_FLAGS -fsanitize=undefined)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    print_file_name("libubsan.so")
  else()
    print_file_name("libclang_rt.ubsan_standalone-x86_64.so")
  endif()
endif()

if(ENABLE_ASAN OR ENABLE_TSAN OR ENABLE_UBSAN)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)

  list(APPEND SANITIZER_CXX_FLAGS -g -fno-omit-frame-pointer)
  list(REMOVE_DUPLICATES SANITIZER_LIBDIRS)

  list(JOIN SANITIZER_CXX_FLAGS    " " SANITIZER_CXX_FLAGS)
  list(JOIN SANITIZER_LINKER_FLAGS " " SANITIZER_LINKER_FLAGS)

  set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} ${SANITIZER_CXX_FLAGS}")
  set(CMAKE_C_FLAGS             "${CMAKE_C_FLAGS} ${SANITIZER_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_LINKER_FLAGS}")

  if(WIN32)
    # Note: sanitizers will probably not work on Windows yet
    list(JOIN SANITIZER_LIBDIRS    ";" SANITIZER_LD_LIBRARY_PATH)
    list(JOIN SANITIZER_LD_PRELOAD ";" SANITIZER_LD_PRELOAD)
  else()
    list(JOIN SANITIZER_LIBDIRS    ":" SANITIZER_LD_LIBRARY_PATH)
    list(JOIN SANITIZER_LD_PRELOAD ":" SANITIZER_LD_PRELOAD)
  endif()
  # For <unknown module> in lsan reports see:
  # https://github.com/google/sanitizers/issues/89#issuecomment-406316683
  # tldr: the module is already gone when the report generator runs
  push_run_env("SANITIZER_LD_PRELOAD" "${SANITIZER_LD_PRELOAD}")
  push_run_env("SANITIZER_LD_LIBRARY_PATH" "${SANITIZER_LD_LIBRARY_PATH}")
endif()
