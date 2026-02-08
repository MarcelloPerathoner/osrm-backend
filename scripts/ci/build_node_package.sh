#!/bin/bash

set -e -o pipefail

BUILD="build/nodejs"
NPM_FLAGS="--directory $BUILD"
if [[ "${BUILD_TYPE:-}" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

echo "node version is:"
which node
node -v
echo "dumping binary meta..."
./node_modules/.bin/node-pre-gyp reveal $NPM_FLAGS
./node_modules/.bin/node-pre-gyp package testpackage $NPM_FLAGS
