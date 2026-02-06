# Building OSRM from Source

## Prerequisites

Check out the repository:

```bash
git clone https://github.com/Project-OSRM/osrm-backend.git
cd osrm-backend
```

The project depends on external libraries. You can install those dependencies with a
package manager (Conan) or install them  manually (apt-get).

## Build manually

Install dependencies:

```bash
sudo apt-get install -y libbz2-dev libxml2-dev libzip-dev liblua5.2-dev libtbb-dev libboost-all-dev
npm ci --ignore-scripts
```
Note: here `ci` means "clean install" and not "continuous integration".

Build:

```bash
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release
make -C build/Release -j 16
```

Replace `16` with the number of cores you have. The binaries will be in `build/Release`.  Proceed with [running the tests](#Run-tests).

A list of arguments for cmake:

| Argument               | Description                                  |
| ---------------------- | -------------------------------------------- |
| `-DMAKE_BUILD_TYPE`    | Specify the build type: `Release` or `Debug` |
| `-DBUILD_SHARED_LIBS`  | Build with shared libs: `ON` or `OFF`.       |
| `-DBUILD_NODE_PACKAGE` | Build the Node package: `ON` or `OFF`.       |

### Node Package

If you want to build the Node package, you should use:

```bash
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release -DBUILD_NODE_PACKAGE=ON
make -C build/Release -j 16
cmake --install build/Release -j 16 --component node_osrm
scripts/ci/build_node_package.sh
```

The node binaries should be in `build/nodejs/lib/binding_napi_v8`.

## Build using Conan

First install Conan. You have to do this only once.

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements*.txt
export CONAN_HOME=`pwd`/.conan2
conan profile detect --force
```

Then say:

```bash
source .venv/bin/activate
export CONAN_HOME=`pwd`/.conan2
conan build -pr home --build=missing -s build_type=Release -o shared=True -o node_package=True
```

N.B. you need to enter the `source` and `export` commands only once per shell
invocation.

A list of arguments for Conan:

| Argument          | Description                                  |
| ----------------- | -------------------------------------------- |
| `-s build_type`   | Specify the build type: `Release` or `Debug` |
| `-o shared`       | Build with shared libs: `True` or `False`.   |
| `-o node_package` | Build the Node package: `True` or `False`.   |

The binaries are now in `build/Release`.

## Run tests

To run the unit tests:

```bash
make -C build/Release -j 16 tests benchmarks
for i in build/Release/unit_tests/*-tests ; do echo Running $i ; $i ; done
```

To run the Cucumber tests:

```bash
npm test -- --parallel 16
```

## The Build Process


The `build` directory is a 'well-known' location for bootstrapping build- and runtime
configurations. Conan puts `conan.env` and Cmake puts `cmake.env` here. `.env` files
should only contain `KEY=VALUE` pairs so they can safely be sourced or piped into
`$GITHUB_ENV`.

The file `conan.env` contains:

| Key                    | Description                                                                 |
| ---------------------- | --------------------------------------------------------------------------- |
| `CONAN_GENERATORS_DIR` | The location where `conan-build-env.sh` and `conan-run-env.sh` are emitted. |
| `CONAN_BUILD_DIR`      | The directory where Cmake should build the binaries.                        |
| `CONAN_CMAKE_PRESET`   | The preset Cmake should use for building.                                   |

After running Conan call Cmake like this:
`cmake -B $CONAN_BUILD_DIR --preset $CONAN_CMAKE_PRESET`


The file `cmake.env` contains:

| Key                         | Description                                          |
| --------------------------- | ---------------------------------------------------- |
| `OSRM_BUILD_DIR`            | The directory where the OSRM binaries will be built. |
| `OSRM_UNIT_TESTS_BUILD_DIR` | The directory where the unit tests will be built.    |
| `OSRM_BENCHMARKS_BUILD_DIR` | The directory where the benchmarks will be built.    |
| `OSRM_NODEJS_BUILD_DIR`     | The directory where the node bindings will be built. |
