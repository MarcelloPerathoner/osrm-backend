# How-To: Build OSRM from Source

> [!NOTE]
> There is also an easy step-by-step [tutorial](tutorial.md).

## Choose a Package Manager

You need some third-party libraries to build OSRM. You can install those libraries
either with:

- a C++ package manager like Conan or,
- a system package manager like `apt-get` or `brew`.

On Windows only the Conan package manager is supported. That doesn't mean there is no
other way, just that we didn't have the time and hardware to figure it out for you.
If you know how to, please submit a patch.

The build method with Conan is tested on Ubuntu-24.04, Ubuntu-22.04, macOS-26, macOS-15,
macOS-14, Windows-Server-2025, and Windows-Server-2022. Conan downloads the sources of
the dependencies and compiles them on your machine. More things can go wrong, it takes
longer, but you always get the newest versions.

The build method with `apt-get` or `brew` is tested on Ubuntu-24.04, Ubuntu-22.04, and
macOS-15. It uses the package manager that comes with the OS. While using a system
package manager has its advantages, there are also some drawbacks to consider. These
include lack of control over the installation process, and the use of outdated versions,
which may be several years old.

> [!NOTE]
> If you switch building methods you must close and reopen the shell, otherwise
> environment variables will clash.

## Prepare

Clone the Github repository and install Node and Python packages:

```bash
git clone https://github.com/Project-OSRM/osrm-backend.git
cd osrm-backend
scripts/post_checkout.sh
```

## Build with Conan

First activate the Python virtual environment. You have to activate it once every time
you open a new shell.

```bash
source scripts/activate_venv
```

Build OSRM with or without the Node package.

::: code-group

```bash [OSRM only]
conan build -pr home --build=missing
```

```bash [OSRM and Node package]
conan build -pr home --build=missing -o node_package=True
```

:::

The OSRM binaries are now in `build/Release` and the Node binaries are now in
`build/nodejs/lib/binding_napi_v8`.

Proceed with testing.

## Build without Conan {#apt}

This build method installs the dependencies with the system package manager and then
builds with `cmake`. This build method is not supported on Windows.

Install dependencies:

::: code-group

```bash [Linux]
sudo apt-get install -y libxml2-dev libbz2-dev libzstd-dev libzip-dev liblua5.2-dev libtbb-dev libboost-all-dev
```

```bash [macOS]
brew install bzip2 libzip lua tbb boost
```

:::

Then build with:

::: code-group

```bash [OSRM only]
cmake -B build/Release
cmake --build build/Release -j
```

```bash [OSRM and Node package]
cmake -B build/Release -DBUILD_NODE_PACKAGE=ON
cmake --build build/Release -j
scripts/ci/build_node_package.sh
```

:::

The OSRM binaries are now in `build/Release` and the Node binaries are in
`build/nodejs/lib/binding_napi_v8`.

## Test

### Unit tests

To run the unit tests:

```bash
cmake --build build/Release -j --target tests
source build/osrm-run-env.sh
ctest --test-dir build/Release -C Release -L tests -j
```

### Cucumber tests {#cucumber}

To run the Cucumber tests:

```bash
npm test -- --parallel 16
```

### Node Package tests {#nodejs-tests}

Only if you built the Node package:

```bash
npm run nodejs-tests
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
