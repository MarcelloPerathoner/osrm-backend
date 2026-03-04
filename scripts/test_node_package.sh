#!/bin/bash

source build/osrm-run-env.sh

make -C "$OSRM_TEST_DATA_DIR"
mkdir -p "$OSRM_TEST_LOG_DIR"
LOG="$OSRM_TEST_LOG_DIR/node-package-test.log"

NPM_FLAGS="--directory $OSRM_NODEJS_INSTALL_PREFIX"
if [[ "$OSRM_CONFIG" == "Debug" ]]; then
    NPM_FLAGS="$NPM_FLAGS --debug"
fi

echo "Y" | "$OSRM_NODEJS_INSTALL_BINDIR/osrm-datastore" --spring-clean
"$OSRM_NODEJS_INSTALL_BINDIR/osrm-datastore" "$OSRM_TEST_DATA_DIR/ch/monaco.osrm"

# ASAN / UBSAN
#
# On macOS SIP will strip DYLD_INSERT_LIBRARIES before calling a child process, so the
# call: `node-pre-gyp testpackage` will not work, because it first calls `/usr/bin/env`
# and then `node --eval(node_osrm.node)`, both of which will strip
# DYLD_INSERT_LIBRARIES.  `node-pre-gyp testpackage` does nothing but try and load the
# native module.  We can skip it altogether because all of the following tests will fail
# if they can't load the native module.

set -x
export LD_PRELOAD=$SANITIZER_LD_PRELOAD
export DYLD_INSERT_LIBRARIES=$SANITIZER_LD_PRELOAD
# export DYLD_PRINT_LIBRARIES=1

node test/nodejs/index.js > "$LOG"

unset LD_PRELOAD
unset DYLD_INSERT_LIBRARIES
unset DYLD_PRINT_LIBRARIES

cat "$LOG" | npx faucet
