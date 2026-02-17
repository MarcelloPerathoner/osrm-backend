# Building OSRM from Source

OSRM depends on external libraries. On Linux you can choose to install those libraries
either with a package manager (Conan) or manually (apt-get). On macOS and Windows only
Conan is supported.

```mermaid
graph LR;
    clone[Clone the repository]
    conan[Build with Conan]
    manually[Build manually<br/>Linux only]
    test[Run tests]
    install[Install]

    clone-->conan-->test;
    clone-->manually-->test;
    test-->install

    click clone "#clone"
    click conan "#conan"
    click manually "#build-manually"
    click test "#tests"
    click install "#install"
```

## Prerequisites {#clone}

Clone the github repository:

```bash
git clone https://github.com/Project-OSRM/osrm-backend.git
cd osrm-backend
npm ci --ignore-scripts
```
Note: here `ci` means "clean install" and not "continuous integration".

## Build using Conan {#conan}

This build method works on Linux, macOS and Windows (Tested on Ubuntu-24.04,
Ubuntu-22.04, macOS-24, macOS-15, macOS-14, Windows-Server-2025, and Windows-Server-2022.)
On Windows a Linux-compatible toolset like mingw is needed.

First install Conan. You have to do this only once after a git clone.

```bash
scripts/install_conan.sh
```

Then say:

```bash
source scripts/activate_conan
conan build -pr home --build=missing
```

The binaries are now in `build/Release`.

Note: you need to activate Conan only once for every shell you open.

A list of arguments for Conan:

| Argument          | Default   | Description                                  |
| ----------------- | --------- | -------------------------------------------- |
| `-s build_type`   | `Release` | Specify the build type: `Release` or `Debug` |
| `-o asan`         | `False`   | Enable Address SANitizer                     |
| `-o assertions`   | `False`   | Enable assertions in release build           |
| `-o ccache`       | `True`    | Use ccache if available                      |
| `-o coverage`     | `False`   | Enable coverage instrumentation              |
| `-o lto`          | `True`    | Enable Link-Time-Optimization                |
| `-o node_package` | `False`   | Build the Node package                       |
| `-o sccache`      | `False`   | Use sccache if available                     |
| `-o shared`       | `False`   | Build with shared libs                       |
| `-o ubsan`        | `False`   | Enable Undefined Behaviour SANitizer         |
| `-o cc`           | standard  | Use this binary as C Compiler                |
| `-o cxx`          | standard  | Use this binary as C++ Compiler              |
| `-o clang-tidy`   | none      | Use this binary as linter                    |

Proceed with [testing](#tests).

## Build manually

This alternative build method is supported on Linux only. (It probably works on macOS
too, if using `brew` instead of `apt-get`. Patches to the docs are welcome.)

Install dependencies:

```bash
sudo apt-get install -y libbz2-dev libxml2-dev libzip-dev liblua5.2-dev libtbb-dev libboost-all-dev
```

Build:

```bash
cmake -B build
make -C build -j
```

The binaries are now in `build`.

A list of arguments for cmake:

| Argument                 | Default   | Description                                         |
| ------------------------ | --------- | --------------------------------------------------- |
| `-DCMAKE_BUILD_TYPE`     | `Release` | Specify the build type: `Release` or `Debug`        |
| `-DCMAKE_C_COMPILER`     | standard  | Which C compiler to use: `clang-19` ...             |
| `-DCMAKE_CXX_COMPILER`   | standard  | Which C++ compiler to use: `clang++-19` ...         |
| `-DCMAKE_CXX_CLANG_TIDY` | none      | Which clang-tidy to use: `clang-tidy-19` ...        |
| `-DBUILD_SHARED_LIBS`    | `OFF`     | Build with shared libs                              |
| `-DBUILD_NODE_PACKAGE`   | `OFF`     | Build the Node package                              |
| `-DBUILD_PACKAGE`        | `OFF`     | Build OSRM package                                  |
| `-DENABLE_ASSERTIONS`    | `OFF`     | Use assertions in release mode                      |
| `-DENABLE_CCACHE`        | `ON`      | Speed up incremental rebuilds via ccache            |
| `-DENABLE_SCCACHE`       | `OFF`     | Speed up incremental rebuilds via sccache           |
| `-DENABLE_LTO`           | `ON`      | Use Link Time Optimization                          |
| `-DENABLE_COVERAGE`      | `OFF`     | Build with coverage instrumentalization             |
| `-DENABLE_DEBUG_LOGGING` | `OFF`     | Use debug logging in release mode                   |
| `-DENABLE_FUZZING`       | `OFF`     | Fuzz testing using LLVM's libFuzzer                 |
| `-DENABLE_ASAN`          | `OFF`     | Use address sanitizer for Debug build               |
| `-DENABLE_TSAN`          | `OFF`     | Use thread sanitizer for Debug build (experimental) |
| `-DENABLE_UBSAN`         | `OFF`     | Use undefined behaviour sanitizer for Debug build   |

Proceed with [testing](#tests).

## Build variations

### Specify compiler

To build with a specified compiler, eg. clang-21 when gcc-15 is the default.

For Conan builds:

```bash
conan build -pr home --build=missing -o cc=clang-21 -o cxx=clang++-21
```

For apt-get builds:

```bash
cmake -B build -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21
make -C build -j
```

### Debug build

To do a debug build.

For Conan builds:

```bash
conan build -pr home --build=missing -s build_type=Debug
```
The binaries will be in `build/Debug`.


For apt-get builds:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
make -C build -j
```
The binaries will be in `build`.

You may also specify `build/Release` or `build/Debug` as build directories, so you can
keep binaries in both directories at the same time.

### Build Node package

To build the Node package, for Conan builds:

```bash
conan build -pr home --build=missing -o node_package=True
```

For apt-get builds:

```bash
cmake -B build -DBUILD_NODE_PACKAGE=ON
make -C build -j
scripts/ci/build_node_package.sh
```
The node binaries are now in `build/nodejs/lib/binding_napi_v8`.

### Other tricks

To learn how OSRM is built on the github CI you may consult the file
`.github/workflows/osrm-backend.yml` and the files under `.github/actions/`.

## Tests {#tests}

### Unit tests {#unit-tests}

To run the unit tests:

```bash
make -C build/Release -j tests benchmarks
ctest --test-dir build/Release/unit_tests/ -j
```

### Cucumber tests {#cucumber}

To run the Cucumber tests:

```bash
npm test -- --parallel 16
```

## Install

```bash
cmake --install build/Release --prefix /usr/local/or/whatever
```


## The Various Build Processes

```mermaid
flowchart TD

    subgraph github [Github]
    direction TB;
        gh_decode[decode_matrix.py]
        gh_presets@{ shape: doc, label: "/<br>CMakePresets.json" }
        gh_apt_get[apt-get install]
        gh_conan[conan install]
        gh_co_presets@{ shape: doc, label: "generators/<br>CMakePresets.json" }
        gh_cm_conf[cmake]
        gh_cm_build[cmake --build]
    end

    gh_decode-->gh_presets-->gh_apt_get--->gh_cm_conf-->gh_cm_build

    gh_presets-->gh_conan-->gh_co_presets-->gh_cm_conf

    subgraph co_build [Conan build]
    direction TB;
        co_install[conan install]
        co_presets@{ shape: doc, label: "generators/<br>CMakePresets.json" }
        co_cm_conf[cmake]
        co_cm_build[cmake --build]
    end

    co_install-->co_presets-->co_cm_conf-->co_cm_build

    subgraph man_build [Manual build]
    direction TB;
        man_apt_get[apt-get install]
        man_cm_conf[cmake]
        man_cm_build[cmake --build]
    end

    man_apt_get-->man_cm_conf-->man_cm_build

```

The `build` directory is a hardcoded 'well-known' location for bootstrapping build- and
runtime configurations.  The `build` directory is used for communication between the
stages of the build toolchain even if the actual build of the binaries should take place
in `build/Release`, `build/Debug`, or elsewhere.

`decode_matrix.py`, Conan and Cmake all put an `.env` file in the `build` directory.
These `.env` files only contain `KEY=VALUE` pairs, so that they can be safely sourced
into a shell, or piped into `$GITHUB_ENV`.

`decode_matrix.py` also puts a file `CMakePresets.json` into the project root directory.
This file can be consumed by CMake or by Conan.

Conan puts a file `CMakePresets.json` into the `generators` directory (that is found
more or less deep in the `build` directory).  This file will be consumed by CMake.

#### build/matrix.env

The file `build/matrix.env` is generated by `decode_matrix.py`. It contains all
information extracted from the github matrix in `KEY=VALUE` pairs. Its main use is for
piping into `$GITHUB_ENV` to make the environment available to all later job steps.

#### build/conan.env

The file `build/conan.env` is generated by Conan. It should be sourced to obtain
information about the build environment setup by Conan. Usage: `source build/conan.env`.
It contains:

| Environment Variable          | Description                                           |
| ----------------------------- | ----------------------------------------------------- |
| `CMAKE_CONFIGURE_PRESET_NAME` | The preset `cmake` should use for configure.          |
| `CMAKE_BUILD_PRESET_NAME`     | The preset `cmake --build` should use.                |
| `CMAKE_TEST_PRESET_NAME`      | The preset `ctest` should use.                        |
| `CONAN_GENERATORS_DIR`        | Where `CMakePresets.json` is emitted. (for debugging) |
| `OSRM_CONFIG`                 | The chosen config: `Release` or `Debug`               |

#### build/osrm-run.env

The file `build/osrm-run-env.sh` is generated by CMake. It can be sourced to obtain
information about the build environment setup by CMake. Usage:
`source build/osrm-run-env.sh`. It contains:

| Environment Variable         | Description                                                |
| ---------------------------- | ---------------------------------------------------------- |
| `OSRM_BUILD_DIR`             | The directory where the OSRM binaries will be built.       |
| `OSRM_UNIT_TESTS_BUILD_DIR`  | The directory where the unit tests will be built.          |
| `OSRM_BENCHMARKS_BUILD_DIR`  | The directory where the benchmarks will be built.          |
| `OSRM_NODEJS_BUILD_DIR`      | The directory where the node bindings will be built.       |
| `OSRM_NODEJS_INSTALL_DIR`    | The directory where the node bindings will be installed.   |
| `OSRM_TEST_DATA_DIR`         | The directory containing the curated test data.            |
| `OSRM_BINARY_DIR`            | 'build' on MSVC 'build/Release' else                       |
| `OSRM_UNIT_TESTS_BINARY_DIR` | 'build/unit_tests' on MSVC 'build/Release/unit_tests' else |
| `OSRM_BENCHMARKS_BINARY_DIR` |                                                            |
| `OSRM_NODEJS_BINARY_DIR`     |                                                            |

Note: multi-config generators (like Visual Studio or Xcode) generate the artifacts in a
`/Release` or `/Debug` subdirectory of the configured build directory. The `OSRM_BUILD_DIR`
variants always include these subdirectories: use them to find the actual files. The
`OSRM_BINARY_DIR` variants do not always include these subdirectories: use them as
cmdline parameters to `cmake` / `ctest`.


#### build/osrm-run-env.sh

The file `build/osrm-run-env.sh` can be sourced to setup a runtime environment in which
the built binaries can be run.

| Environment Variable | Description                                         |
| -------------------- | --------------------------------------------------- |
| `LD_LIBRARY_PATH`    | The path to the libraries built by Conan. (Linux)   |
| `DYLD_LIBRARY_PATH`  | The path to the libraries built by Conan. (macOS)   |
| `PATH`               | The path to the libraries built by Conan. (Windows) |
