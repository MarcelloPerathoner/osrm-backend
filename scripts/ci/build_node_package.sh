#!/bin/bash

STAGE_DIR=build/nodejs

set -e -o pipefail

source build/osrm-run.env

echo "node version is:"
which node
node -v

NPM_FLAGS="--directory $STAGE_DIR"
if [[ "$OSRM_CONFIG" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

for comp in node_package applications libraries data; do
    set -x
    cmake --install "${CMAKE_BINARY_DIR}" --config "${OSRM_CONFIG}" --component $comp --prefix $STAGE_DIR -v
    set +x
done

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS
./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
