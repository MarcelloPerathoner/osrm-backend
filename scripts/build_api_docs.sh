#!/bin/bash

set -e -u

if [ ! -f docs/http.md ] ; then
    echo "This script should be run from the repository root directory"
    exit 1
fi

# Clean up previous version
rm -rf build/docs
mkdir -p build/docs/tmp build/docs/css

# Extract JSDoc comments from C++ and generate API docs
# (replaces --polyglot flag removed in documentation.js v14)
node scripts/extract_cpp_jsdoc.js src/nodejs/node_osrm.cpp > build/docs/tmp/jsdoc-extract.js

# turn extracted JSDoc into markdown
echo "# OSRM NodeJS API" > build/docs/tmp/nodejs.md
npx -- documentation build build/docs/tmp/jsdoc-extract.js \
                --markdown-toc=false -f md >> build/docs/tmp/nodejs.md

# Copy our images and css into the temp dir
cp -r docs/*.md           build/docs/tmp
cp -r docs/images         build/docs/
cp    node_modules/github-markdown-css/github-markdown.css build/docs/css/
cp    node_modules/highlight.js/styles/github.css          build/docs/css/highlight-js.css

# Build the HTML files
for md in docs/*.md build/docs/tmp/nodejs.md
do
  node docs/src/markdown2html.js -t docs/src/index.template.html -i "$md" > "build/docs/$(basename $md).html"
done

# Cleanup
rm -rf build/docs/tmp
