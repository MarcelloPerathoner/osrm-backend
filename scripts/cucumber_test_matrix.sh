#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

loadmethods=(datastore mmap directly)
profiles=(ch mld)

for profile in "${profiles[@]}"
do
  for loadmethod in "${loadmethods[@]}"
  do
    set -x
    OSRM_LOAD_METHOD=$loadmethod ROUTING_ALGORITHM=$profile npx cucumber-js features/ -p $profile --parallel ${JOBS:=8}
    { set +x; } 2>/dev/null
  done
done
