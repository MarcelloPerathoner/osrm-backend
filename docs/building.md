# Building OSRM from Source

## Prerequisites

Start with checking out the repository:

```bash
git clone https://github.com/Project-OSRM/osrm-backend.git
cd osrm-backend
```

The project depends on some external libraries. You can install those dependencies with
a package manager (Conan) or install them  manually.

## Build manually

Install dependencies:

```bash
sudo apt-get install -y libbz2-dev libxml2-dev libzip-dev liblua5.2-dev libtbb-dev libboost-all-dev chrpath
npm ci --ignore-scripts
```
Note: here `ci` means "clean install" and not "continuous integration".

Build:

```bash
cmake -B build/Release -DCMAKE_BUILD_TYPE=Release
make -C build/Release -j 16 all tests benchmarks
```

The binaries should be in `build/Release`.  Proceed with [running the tests](#Run-tests).

If you want to build the node package, you should use:

```bash
node run install
```

The node binaries should be in `lib/binding_napi_v8`.

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
conan build -pr home --build=missing -s build_type=Release -o shared=True -o node_bindings=True
make -C build/Release -j 16 tests benchmarks
```

N.B. you need the `source` and `export` commands only once per shell invocation.

The binaries are now in `build/Release`.

## Run tests

To run the unit tests:

```bash
for i in build/Release/unit_tests/*-tests ; do echo Running $i ; $i ; done
```

To run the Cucumber tests:

```bash
npm test -- --parallel 16
```
