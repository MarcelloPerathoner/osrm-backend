#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

loadmethods=(datastore mmap directly)
algorithms=(ch mld)

for algorithm in "${algorithms[@]}"
do
  for loadmethod in "${loadmethods[@]}"
  do
    set -x
    npx cucumber-js features/ -p $loadmethod -p $algorithm \
        --format "html":"test/logs/cucumber-$algorithm-$loadmethod.report.html" \
        --format "message":"test/logs/cucumber-$algorithm-$loadmethod.report.json" \
        $@
    { set +x; } 2>/dev/null
  done
done
