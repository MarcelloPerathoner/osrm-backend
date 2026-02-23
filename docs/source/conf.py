# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os

import docutils
from docutils import nodes
from docutils.nodes import Element
from docutils.parsers.rst.directives import images
from docutils.parsers.rst.roles import register_generic_role, register_local_role

from sphinx import addnodes
from sphinx.locale import admonitionlabels
from sphinx.roles import ReferenceRole, SphinxRole, EmphasizedLiteral
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective
from sphinx.writers.html5 import HTML5Translator

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
# html_show_copyright = False
html_show_sourcelink = False
html_show_sphinx = False
# html_logo = "logo.svg"

# make `backticked text` mean: code
default_role = "code"


class DefaultRole(SphinxRole):
    pass


class CustomReferenceRole(ReferenceRole):
    """Roles to reference targets on the www"""

    nodeclass: type[Element] = nodes.reference

    def get_title(self):
        """Return the title"""
        if not self.has_explicit_title:
            return self.target
        return self.title

    def get_reference_option(self):
        """Return a dict with refuri and classes"""
        raise NotImplementedError

    def mk_node(self):
        return self.nodeclass(
            "",
            self.get_title(),
            internal=False,
            **self.get_reference_options(),
        )

    def run(self):
        return [self.mk_node()], []


class GithubIssueRole(CustomReferenceRole):
    def get_reference_options(self):
        return {
            "refuri": f"https://github.com/Project-OSRM/osrm-backend/issues/{self.target}",
            "classes": ["github", "github-issue"],
        }

    def get_title(self):
        """Return the title"""
        if not self.has_explicit_title:
            return f"issue #{self.target}"
        return self.title


class GithubPullRequestRole(CustomReferenceRole):
    def get_reference_options(self):
        return {
            "refuri": f"https://github.com/Project-OSRM/osrm-backend/pull/{self.target}",
            "classes": ["github", "github-pull-request"],
        }

    def get_title(self):
        """Return the title"""
        if not self.has_explicit_title:
            return f"PR #{self.target}"
        return self.title


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


def setup(sphinx):
    sphinx.add_lexer("endpoint", EndpointLexer)
    # sphinx.add_domain(SomeDomain)
    sphinx.add_role("issue", GithubIssueRole())
    sphinx.add_role("pull", GithubPullRequestRole())
    sphinx.add_role("pr", GithubPullRequestRole())
    # app.add_role("default", DefaultRole())
    # register_generic_role("default", nodes.strong)
    register_local_role("default", EmphasizedLiteral())  # type: ignore[arg-type]
