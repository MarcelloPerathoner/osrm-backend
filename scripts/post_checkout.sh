#!/usr/bin/env bash
#
# This script should be called from the project root directory once immediately after a
# git clone. It installs required dependencies.
#
# usage: scripts/post_checkout.sh

# install node pacakges
# 'ci' here means 'clean install' and not 'continuous integration'.
# It install the packages exactly as in package-lock.json.
npm ci --ignore-scripts

# install python packages
scripts/install_venv.sh
