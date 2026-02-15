#!/usr/bin/env bash

# call this from the project root directory
# use once after a git clone
# usage: scripts/install-conan.sh

pip install -r requirements*.txt
python -m venv .venv
source activate_conan
conan profile detect --force
