# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os

from pygments.lexer import RegexLexer, bygroups
from pygments.token import Whitespace, Text, Keyword, Literal

project = "OSRM"
copyright = "2026, OSRM contributors"
author = "OSRM contributors"
release = "6.0.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "myst_parser",  # parses markdown
    "sphinx_design",
    "sphinx_rtd_theme",
    "sphinxcontrib.httpdomain",
    "sphinxcontrib.mermaid",
]
exclude_patterns = []

templates_path = ["_templates"]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

myst_heading_anchors = 4
myst_enable_extensions = [
    "amsmath",
    "attrs_inline",
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
    "html_admonition",
    "html_image",
    "linkify",
    "replacements",
    "smartquotes",
    "strikethrough",
    "substitution",
    "tasklist",
]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_favicon = "images/favicon.ico"
html_static_path = ["_static"]
html_css_files = ["css/osrm-custom.css"]

# make `backticked text` mean: code
default_role = "code"


class EndpointLexer(RegexLexer):
    """A Pygments Lexer. Look for ```endpoint in the docs."""

    def curly_callback(self, match, ctx):
        text = match.group(0)
        tok = Keyword
        i = match.start()

        while "|" in text:
            tok = Literal
            s, text = text.split("|", 1)
            yield i, tok, s
            i = i + len(s)
            yield i, Text, "|"
            i = i + 1
        yield i, tok, text

    tokens = {
        "root": [
            (r"\s+", Whitespace),
            (r"GET|POST", Keyword),
            (r"([&?])(\w+)(=)", bygroups(Text, Keyword, Text)),
            (r"({)([\w,|]+)(})", bygroups(Text, curly_callback, Text)),
            (r"[^{|&?]+", Text),
        ],
    }


def setup(sphinx):
    sphinx.add_lexer("endpoint", EndpointLexer)
    # sphinx.add_domain(SomeDomain)


# pre-flight: extract comments from node_osrm.cpp

with open("../../src/nodejs/node_osrm.cpp", "r") as cpp:
    lines = []
    in_comment = False
    for line in cpp:
        if line.find("*/") >= 0:
            in_comment = False
            lines.append("\n\n")
        if in_comment:
            lines.append(line)
        if line.find("/**") >= 0:
            in_comment = True

    gen_dir = "../generated/"
    os.makedirs(gen_dir, exist_ok=True)
    with open(os.path.join(gen_dir, "node_osrm.rst"), "w") as rst:
        rst.writelines(lines)
