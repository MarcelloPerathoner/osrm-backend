#!/bin/bash

set -e
set -o pipefail

BUILD="build/nodejs"
BINDINGS="$BUILD/lib/binding_napi_v8"
NODE_OSRM="node_osrm.node"
ELF_OUT="$BUILD/readelf-output.txt"

mkdir -p "$BINDINGS"

echo "node version is:"
which node
node -v

NPM_FLAGS="--directory $BUILD"
if [[ "${BUILD_TYPE:-}" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

# copy files into BINDINGS

source build/cmake-run-env.sh
cmake --install "$OSRM_BUILD_DIR" --component node_osrm -v

# cp package.json "$BUILD"
# cp src/nodejs/index.js "$BUILD/lib"
# cp "$OSRM_NODEJS_BUILD_DIR/$NODE_OSRM" "$BINDINGS"
# for n in components contract customize datastore extract partition routed ; do
#     cp -v "$OSRM_BUILD_DIR/osrm-$n${EXE:-}" "$BINDINGS"
# done
#
# DEPS=`python scripts/ci/runtime_dependencies.py --grep "boost|bz2|tbb|osrm" "$OSRM_NODEJS_BUILD_DIR/$NODE_OSRM"
# echo "=== Dependencies ==="
# echo "$DEPS"
# echo "===================="
#
# # cp on macOS knows no -t, no -u
# echo "$DEPS" | xargs -I '{}' cp -v '{}' "$BINDINGS" || true
#
# case $(uname) in
#   Linux)
#     ldd "$BINDINGS/$NODE_OSRM"
#     chrpath --replace '$ORIGIN' "$BINDINGS/$NODE_OSRM"
#   ;;
#   Darwin)
#     otool -l "$BINDINGS/$NODE_OSRM"
#     otool -L "$BINDINGS/$NODE_OSRM"
#   ;;
#   Windows)
#     DUMPBIN /DEPENDENTS "$BINDINGS/$NODE_OSRM"
#   ;;
# esac

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS
./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
