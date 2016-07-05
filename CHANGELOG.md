## Upcoming

### Buildsystem

* New method of handling external projects

### Library

* Completely redone comment parsing, add ability to use almost arbitrary Markdown in the documentation comments

* More output formats

* New commmand: `exclude` to exclude entites from the output via comment

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
