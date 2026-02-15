/**
 * Converts a markdown file into an HTML file.
 *
 * The design goal is to be most compatible with the markdown processing done on github,
 * supporting the same features and yielding the same look.
 *
 * - highlight code blocks
 * - highlight the custom ```endpoint code block
 * - transform links to .md files into links to .html files
 * - support mermaid graphics
 */

import fs from 'node:fs';

import { Marked } from 'marked';
import { markedHighlight } from 'marked-highlight';
import { gfmHeadingId } from 'marked-gfm-heading-id';
import customHeadingId from 'marked-custom-heading-id';
import markedAlert from 'marked-alert';
import markedLinkifyIt from 'marked-linkify-it';
import hljs from 'highlight.js';
import hljsCurl from 'highlightjs-curl';
import Handlebars from 'handlebars';
import yargs from 'yargs';
import { hideBin } from 'yargs/helpers';

const argv = yargs(hideBin(process.argv))
  .usage('Usage: $0 --template template.html --input doc.md > doc.html')
  .option('template', {
    alias: 't',
    type: 'string',
    required: true,
    description: 'The html template (parsed with handlebars)'
  })
  .option('input', {
    alias: 'i',
    type: 'string',
    required: true,
    description: 'The input file (markdown)'
  })
  .parse();

const marked = new Marked();

hljs.registerLanguage('curl', hljsCurl);
marked.use(
  markedHighlight({
    emptyLangClass: 'hljs',
    langPrefix: 'hljs language-',
    highlight(code, lang, _info) {
      const language = hljs.getLanguage(lang) ? lang : 'plaintext';
      return hljs.highlight(code, { language }).value;
    }
  })
);
marked.use(gfmHeadingId());
marked.use(markedAlert());
marked.use(customHeadingId()); // after gfmHeadingId() ! so it takes precedence
marked.use(markedLinkifyIt({}, {}));

/** The document <title>. The first <h1> will become the title. */
let title = null;
/** Set true if a mermaid codeblock was found. */
let needs_mermaid = false;

const tokenizer = {
  /** Turns a link to file.md into a link to file.html */
  link(src) {
    const match = src.match(/^\[(.+?)\]\((.+?)\.md\)/);
    if (match) {
      const token = {
        type: 'link',
        raw: match[0],
        href: `${match[2]}.html`,
        text: match[1],
        tokens: []
      };
      this.lexer.inline(token.text, token.tokens);
      return token;
    }
    // return false to use original tokenizer
    return false;
  }
};

const renderer = {
  code(token) {
    // highlight an ```endpoint custom code block
    if (token.lang === 'endpoint') {
      const parts = token.text.split(/\s(.*)/s);
      const attrs = parts[1].replace(/{([\w_]+)}/g, (s) => `<span class="hljs-attr">${s}</span>`);
      return `<pre><code class='hljs language-endpoint'><span class='hljs-keyword'>${parts[0]}</span> ${attrs}</code></pre>`;
    }
    // support mermaid graphics
    if (token.lang === 'mermaid') {
      needs_mermaid = true;
      return `<pre class='mermaid'>${token.text}</pre>`;
    }
    return false;
  },
  heading(token) {
    // set the <title> to the first <h1>
    if (token.depth == 1 && title === null) {
      title = token.text;
    }
    return false;
  }
};

marked.use({
  async: false,
  pedantic: false,
  gfm: true,  // use github markup extensions
  tokenizer,  // add our tokenizer
  renderer,   // add our renderer
});

title = title || 'OSRM Documentation';

const template = Handlebars.compile(fs.readFileSync(argv.template, 'utf8'));
const article = marked.parse(fs.readFileSync(argv.input, 'utf8'));

const html = template({ title, article, needs_mermaid });

console.log(html);
