#!/usr/bin/env bash
#
# This script installs the python packages from requirements*.txt and inits Conan.
#
# usage: scripts/install_venv.sh

pip install -r requirements*.txt
python -m venv .venv
source scripts/activate_venv

export CONAN_HOME=`pwd`/.conan2
conan profile detect --force
