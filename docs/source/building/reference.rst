OSRM Building Reference
=======================

Building with Conan
-------------------

Examples:

.. code:: bash

    conan build -pr home --build=missing -s build_type=Debug
    conan build -pr home --build=missing -o cc=clang-21 -o cxx=clang++-21
    conan build -pr home --build=missing -o shared=True -o node_package=True

Arguments:

================= ========= ============================================
Argument          Default   Description
================= ========= ============================================
`-s build_type`   `Release` Specify the build type: `Release` or `Debug`
`-o asan`         `False`   Enable Address SANitizer
`-o assertions`   `False`   Enable assertions in release build
`-o ccache`       `True`    Use ccache if available
`-o coverage`     `False`   Enable coverage instrumentation
`-o lto`          `True`    Enable Link-Time-Optimization
`-o node_package` `False`   Build the Node package
`-o sccache`      `False`   Use sccache if available
`-o shared`       `False`   Build with shared libs
`-o ubsan`        `False`   Enable Undefined Behaviour SANitizer
`-o cc`           standard  Use this binary as C Compiler
`-o cxx`          standard  Use this binary as C++ Compiler
`-o clang-tidy`   none      Use this binary as linter
`-o generator`    standard  Use this Cmake generator: `Ninja`
================= ========= ============================================


Building with CMake
-------------------

Examples:

.. code:: bash

    cmake -B build -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake -B build -DBUILD_NODE_PACKAGE=ON


======================== ========= ===================================================
Argument                 Default   Description
======================== ========= ===================================================
`-DCMAKE_BUILD_TYPE`     `Release` Specify the build type: `Release` or `Debug`
`-DCMAKE_C_COMPILER`     standard  Which C compiler to use: `clang-19` ...
`-DCMAKE_CXX_COMPILER`   standard  Which C++ compiler to use: `clang++-19` ...
`-DCMAKE_CXX_CLANG_TIDY` none      Which clang-tidy to use: `clang-tidy-19` ...
`-DBUILD_SHARED_LIBS`    `OFF`     Build with shared libs
`-DBUILD_NODE_PACKAGE`   `OFF`     Build the Node package
`-DBUILD_PACKAGE`        `OFF`     Build OSRM package
`-DENABLE_ASSERTIONS`    `OFF`     Use assertions in release mode
`-DENABLE_CCACHE`        `ON`      Speed up incremental rebuilds via ccache
`-DENABLE_SCCACHE`       `OFF`     Speed up incremental rebuilds via sccache
`-DENABLE_LTO`           `ON`      Use Link Time Optimization
`-DENABLE_COVERAGE`      `OFF`     Build with coverage instrumentalization
`-DENABLE_DEBUG_LOGGING` `OFF`     Use debug logging in release mode
`-DENABLE_FUZZING`       `OFF`     Fuzz testing using LLVM's libFuzzer
`-DENABLE_ASAN`          `OFF`     Use address sanitizer for Debug build
`-DENABLE_TSAN`          `OFF`     Use thread sanitizer for Debug build (experimental)
`-DENABLE_UBSAN`         `OFF`     Use undefined behaviour sanitizer for Debug build
`-G`                     standard  Use CMake generator, eg.: `-G Ninja`
======================== ========= ===================================================

Other tricks
------------

To learn how OSRM is built on the github CI you may consult the file
`.github/workflows/osrm-backend.yml` and the files under `.github/actions/`.
