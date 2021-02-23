# Render LaTeX in GitHub Pages with MathJax

This setup runs standardese and then processes the generated Markdown with
Jekyll like GitHub Pages does.  To start a local web server serving the
generated static HTML, run the following:

```
cd doc
make
```

This assumes that standardese and Ruby's [bundler](https://bundler.io/) are on
your path.

## Supported LaTeX Delimiters

In this setup we support `$ … $` and `\\[ … \\]` delimited LaTeX for inline and
equations, respectively. Note that other flavours, such as `$$ … $$` might get
escaped by kramdown's MathJax support which breaks subscripts since these then
get doubly escaped. To fix this, you can either use a different implementation,
e.g., by setting `markdown: GFM` in `_config.yml` or disable kramdown's math
engine completely:

```
kramdown:
  math_engine: ~
```

However, the latter does not work when using gh-pages since they [override this
again](https://github.com/github/pages-gem/blob/9be43d0c89b6d6e93f74abe9a997d6a025d94a43/lib/github-pages/configuration.rb#L57).
