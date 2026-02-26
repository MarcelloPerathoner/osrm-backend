# Build OSRM from Source

This tutorial explains how to build OSRM from source. It assumes familiarity with the
shell, and you must have `git`, `cmake`, and a C++ compiler already installed. On
Windows it assumes a Linux-compatible toolset like MSYS2 is installed.

> [!NOTE]
> More information and alternative build methods can be found in the [How-To](howto.md).

## Prepare

Clone the Github repository and install Node and Python packages:

```bash
git clone https://github.com/Project-OSRM/osrm-backend.git
cd osrm-backend
scripts/post_checkout.sh
```

## Build

First activate the Python virtual environment. You have to activate it once every time
you open a new shell.

```bash
source scripts/activate_venv
```

Then build OSRM:

```bash
conan build -pr home --build=missing
```

The OSRM binaries are now in `build/Release`.

## Test

### Cucumber tests {#cucumber}

To run the Cucumber tests:

```bash
npm test -- --parallel 16
```

### Unit tests

If you want to run the unit tests:

```bash
cmake --build build/Release -j --target tests
source build/osrm-run-env.sh
ctest --test-dir build/Release -C Release -L tests -j
```

## Install

Install OSRM into system directories (you need `sudo`-rights):

```bash
sudo cmake --install build/Release --config Release
```

Or, install OSRM into user directories:

```bash
cmake --install build/Release --config Release --prefix ~/my/install/dir
```

> [!NOTE]
> More information and alternative build methods can be found in the [How-To](howto.md).
