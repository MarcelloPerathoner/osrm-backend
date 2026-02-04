#!/bin/bash

set -e
set -o pipefail

BINDINGS="./lib/binding_napi_v8"
NODE_OSRM="node_osrm.node"
ELF_OUT="build/readelf-output.txt"

echo "node version is:"
which node
node -v

NPM_FLAGS=''
if [[ "${BUILD_TYPE:-}" == "Debug" ]]; then
    NPM_FLAGS='--debug'
fi

# copy files into BINDINGS

source build/cmake.env

mkdir -p "$BINDINGS"
cp "$OSRM_NODEJS_BUILD_DIR/$NODE_OSRM" "$BINDINGS"
for n in components contract customize datastore extract partition routed ; do
    cp -v "$OSRM_BUILD_DIR/osrm-$n${EXE:-}" "$BINDINGS"
done

# Copy dynamic library dependencies. Note: the RPATH inside node_osrm.node is not enough
# because it is not valid for recursively loaded libraries: when boost_random imports
# boost_system there is no RPATH in boost_random.
if [[ "$CONAN_GENERATORS_DIR" != "" ]]; then
    source "$CONAN_GENERATORS_DIR/conan-run-env.sh"
    export LD_LIBRARY_PATH DYLD_LIBRARY_PATH PATH
fi

python scripts/ci/runtime_dependencies.py --grep "boost|bz2|tbb|osrm" "$BINDINGS/$NODE_OSRM" | \
    while IFS= read -r line; do cp -uv "$line" "$BINDINGS" || true; done

if [[ $(uname -s) == 'Linux' ]]; then
  chrpath --replace '$ORIGIN' "$BINDINGS/$NODE_OSRM"
fi

# echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS

# enforce that binary has proper ORIGIN flags so that
# it can portably find libtbb.so in the same directory
if [[ $(uname -s) == 'Linux' ]]; then
    readelf -d "$BINDINGS/$NODE_OSRM" > "$ELF_OUT"
    if grep -q 'Flags: ORIGIN' "$ELF_OUT"; then
        echo "Found ORIGIN flag in readelf output"
        cat "$ELF_OUT"
    else
        echo "*** Error: Could not find ORIGIN flag in readelf output"
        cat "$ELF_OUT"
        exit 1
    fi
fi

./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
