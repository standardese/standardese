## Upcoming

### Buildsystem

* New method of handling external projects

### Documentation

* Add ability to use almost arbitrary Markdown in the documentation comments

* Link to other entities via `[link-text](<> "unique-name")`, where `unique-name` is the name of the entity with all scopes and (template) parameters, `link-text` is the arbitrary text of the link and the literal `<>` denotes an empty URL.
 This can be abbreviated to `[unique-name]()` if you don't need a special text.

* New commmand: `exclude` to exclude entites from the output via comment

* New command: `unique_name` to override the unique name of an entity

### Library

* New `md_entity` hierachy

* Redid comment parsing and sectioning

* Redid outputting, new `output_format_base` hierachy, new `output` class merely acting as wrapper; `code_block_writer` now standalone class

* New functions in `cpp_entity`

* Internal changes

### Tool

* Multithreaded documentation generation and new option `jobs/j` to specify number of threads.

* New options:  `input.blacklist_dotfiles`, `comment.implicit_paragraph`, `output.format`, `output.width` and `output.tab_width`

## 0.1-1

Bugfixes, better compiler support.

## 0.1

### Buildsystem

* changed target names to reflect namespaces

* installation options

* support for libclang 3.8

* CMake integration for building with `standardese_generate()`

### Library:

* complete restructuring (seperation of comments and entity, multiple configurations)

* `standardese::compile_config` class for compilation options and `compile_commands.json` support

* new parsing with the help of Boost.Wave
 
* skip attributes when parsing

* more robust parsing, error handling options

* `standardese::entity_blacklist` to blacklist entities for synopsis and generation

* a couple of utility functions

* better detection of overridden `virtual` functions

* support for libclang 3.8 and template aliases

* many internal changes and bugfixes

### Tool

* new options for compilation and entity filtering

* verbose output and coloring options

## 0.0

First basic prototype.
