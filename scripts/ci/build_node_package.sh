#!/bin/bash

set -e -o pipefail

NPM_FLAGS="--directory build/nodejs"
if [[ "${BUILD_TYPE:-}" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

echo "node version is:"
which node
node -v

cmake --install "${OSRM_BUILD_DIR}" --component node_osrm -v

echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS
./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
