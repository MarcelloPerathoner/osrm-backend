# Build the docs
#
# This Makefile builds the OSRM docs. In particular it:
#
# - extracts /** */ style comments from node_osrm.cpp and turns them into markdown
# - converts all markdown files into html files
# - copies css and image files
#
# Read the docs:
#
#   browser build/docs/index.html

PROJECT=..

DOCS=$(PROJECT)/docs
BUILD=$(PROJECT)/build/docs
NODE=$(PROJECT)/node_modules
SCRIPTS=$(PROJECT)/scripts

MDS=$(shell find $(DOCS) -name "*.md") $(DOCS)/generated/nodejs.md
HTMLS=$(patsubst %,$(BUILD)/%.html,$(basename $(MDS)))
DIRS=$(patsubst %,$(BUILD)/%,$(dir $(MDS)))

all: dirs docs aux

dirs:
	mkdir -p $(DIRS)

docs: $(MDS) $(HTMLS)

vpath %.md $(DOCS):$(DOCS)/generated

# Extract JSDoc comments from cpp file
$(BUILD)/nodejs.jsdoc: $(PROJECT)/src/nodejs/node_osrm.cpp $(SCRIPTS)/extract_cpp_jsdoc.js
	node $(SCRIPTS)/extract_cpp_jsdoc.js $< > $@

# turn extracted JSDoc into markdown
$(DOCS)/generated/nodejs.md : $(BUILD)/nodejs.jsdoc
	echo "# OSRM NodeJS API" > $@
	npx -- documentation build $< --markdown-toc=false -f md >> $@

# Build the HTML files
$(BUILD)/%.html : %.md $(DOCS)/templates/article.html $(SCRIPTS)/markdown2html.js
	node $(SCRIPTS)/markdown2html.js -t $(DOCS)/templates/article.html -b $(BUILD) -i $< -o $@

# Copy images and css into the build dir
aux:
	cp -r $(DOCS)/css    $(BUILD)/
	cp -r $(DOCS)/images $(BUILD)/
	cp $(NODE)/github-markdown-css/github-markdown.css $(BUILD)/css/
	cp $(NODE)/highlight.js/styles/github.css          $(BUILD)/css/highlight-js.css
