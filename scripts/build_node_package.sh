#!/bin/bash

set -e -o pipefail

source build/osrm-run-env.sh

echo "node version is:"
which node
node -v

set -x
cmake --install "$CMAKE_BINARY_DIR" --config "$OSRM_CONFIG" --component node_package --prefix "$OSRM_NODEJS_INSTALL_PREFIX" -v
set +x

NPM_FLAGS="--directory $OSRM_NODEJS_INSTALL_PREFIX"
if [[ "$OSRM_CONFIG" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS
./node_modules/.bin/node-pre-gyp package $NPM_FLAGS
