/**
 * Converts a markdown file into an HTML file.
 *
 * It tries to give an output very similar to markdown rendered on github.  In
 * particular it highlights code blocks and transforms links to .md files into links to
 * .html files.
 */

import fs from 'node:fs';

import Handlebars from 'handlebars';
import yargs from 'yargs';
import { hideBin } from 'yargs/helpers';
import { Marked } from 'marked';
import { markedHighlight } from 'marked-highlight';
import { gfmHeadingId } from 'marked-gfm-heading-id';
import markedAlert from 'marked-alert';
import hljs from 'highlight.js';
import hljsCurl from'highlightjs-curl';

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

const marked = new Marked(
  markedHighlight({
    emptyLangClass: 'hljs',
    langPrefix: 'hljs language-',
    highlight(code, lang, _info) {
      const language = hljs.getLanguage(lang) ? lang : 'plaintext';
      return hljs.highlight(code, { language }).value;
    }
  })
);

const tokenizer = {
  /** Turns a link to [...](file.md) into a link to file.md.html */
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

function highlightAttrs(str) {
  return str.replace(/{([\w_]+)}/g, (str) => `<span class="hljs-attr">${str}</span>`);
}

const renderer = {
  /** Highlights an ```endpoint code block */
  code(token) {
    if (token.lang === 'endpoint') {
      const parts = token.text.split(/\s(.*)/s);
      return `<pre><code class='hljs language-endpoint'><span class='hljs-keyword'>${parts[0]}</span> ${highlightAttrs(parts[1])}</code></pre>`;
    }
    return false;
  }
};

marked.use({ tokenizer });
marked.use({ renderer });
marked.use(gfmHeadingId());
marked.use(markedAlert());

marked.use({
  async: false,
  pedantic: false,
  gfm: true,
});

hljs.registerLanguage('curl', hljsCurl);
const template = Handlebars.compile(fs.readFileSync(argv.template, 'utf8'));
const article = marked.parse(fs.readFileSync(argv.input, 'utf8'));

const title = 'OSRM Documentation';
const html = template({ title, article });

console.log(html);
