=====================
standardese Change Log
=====================

.. current developments

v0.5.0
====================

**Added:**

* `--comment.free_file_comments` flag so that comments that cannot be
  associated to any entity are automatically considered to be referring to the
  entire file even if there's no `\file` command. (The default is still the old
  behaviour. Such comments are ignored with a warning.)
* Added command line option `--group-uncommented` to add uncommented members to
  the group of the preceding member, e.g., here both operators are put in the
  same group `Arithmetic` without having to explicitly mention the group for
  the second line.
  ```cpp
  struct S {
      /// \group Arithmetic
      S& operator+=(const S&);
      S& operator-=(const S&);
  }
  ```
* new flag `--input.hide_undocumented` to completely hide undocumented members
  from class summary without having to explicitly put `\exclude` everywhere.
* Added an example project using MathJax on GitHub Pages.
* Added option `--comment.command_pattern` to change or extend the commands
  recognized by Standardese. Use `name=pattern` to replace the pattern for
  `name` and `name|=pattern` to extend the default pattern. E.g.,
  ```
  --comment.command_pattern 'returns|=RETURNS:'
  ```
  lets the parser recognize the string `RETURNS:` at the beginning and the
  string `\returns` as introducing a `Return values:` section.
  If the command takes parameters, they have to be caught in capturing groups, e.g.,
  ```
  --command.command_pattern 'group=//== ([^\n]*)() ==//'
  ```
  allows us to write
  ```
  //== Group Heading ==//
  ```
  to introduce a member group instead of `\group name Group Heading`. Note
  that a group has two parameters. The second parameter is the optional
  heading which we ignore here by letting it capture the empty string.

**Changed:**

* Require a C++17 compiler to build standardese
* Sections do not treat entries of the form `\section key - text` specially
  anymore. This created a substantial amount of complexity during parsing and
  also only did not have full Markdown support in the `key` section. Instead,
  one should just use a proper Markdown list instead.

* Moved `section_type` from `standardese::markup` to the `standardese::comment`
  namespace where `command_type` and `inline_type` are already.
* renamed CHANGELOG to CHANGELOG.rst so it renders better on GitHub

**Removed:**

* Removed command line parameters related to templates, since templates are not
  implemented.

* Removed command line parameter `output.section_name_` since it was not
  implemented.

* Removed `command_type::verbatim`. `\verbatim` is still supported but not
  treated as a `command` internally anymore (which is never really was anyway.)

* Removed `command_type::invalid`, `inline_type::invalid`,
  `section_type::invalid` since these are never created anymore.

* Removed `standardese::markup::doc_section::type()` since it is not used anymore.

* Removed helper functions `standardese::comment::is_section`,
  `standardese::comment::make_section`, `standardese::comment::is_command`,
  `standardese::comment::make_command`, `standardese::comment::is_inline`,
  `standardese::comment::make_inline`. These functions are not needed anymore
  since the related enums are now separate and do not form part of a increasing
  sequence anymore. (There was very little code where this was actually
  helpful.)

* Various `set_` commands on `standardese::comment::config` since config is now
  immutable.

**Fixed:**

* Added a missing mutex lock in the comment parser.
* `make install` now installs the standardese binary
* Do not mention templating in README or command line help since it is not implemented.

* Check all return values from calls into cmark (by always going through an
  exception-throwing wrapper.)

* Simplified CommonMark extensions by not using any temporary nodes.
* Build as shared library and link against a shared boost library unless told
  not to.
* Catch headers for testing are now taken from a submodule. Before we had
  problems frequently, e.g., in CI, when downloading these headers failed.



v0.4.1
====================

**Changed:**

* releases on GitHub are now created semi-automatically with [rever](https://github.com/regro/rever)

**Fixed:**

* catch header URL when downloading catch during test build
* docker build; the docker build had stopped working a few months ago, since Ubuntu 19.04 had reached its end of life.
* assertion error with unnamed namespaces



v0.4.0
====================

* Complete internal rewrite using [cppast](https://github.com/foonathan/cppast) and GitHub's fork of [cmark](https://github.com/github/cmark-gfm). Note that templating has not been implemented yet.

**Build System:**

* We do not release static binaries anymore but standardese should be soon on [conda-forge](https://conda-forge.org/) and there is a [Docker image](https://hub.docker.com/r/standardese/standardese) with a built binary.

v0.3-4
====================

**Documentation:**

* Hide member group output section by prefixing the group name with `-`
* Hide name lookup character in entity link

**Library:**

* Now links to external entities in advanced code block
* Only links the main part of type name (i.e. in `const T*const &`, only link will be the `T`)
* Even more bugfixes

**Tool:**

* New option `output.require_comment_for_full_synopsis`, controls whether full synopsis will be shown for classes/enums without documentation (disabled by default)

v0.3-3
====================

**Documentation:**

* Optional arguments for `exclude` to specify what to exclude (return type, target)
* Advanced code block does not generate links to parameters now

**Library:**

* Bugfixes in various functions

**Build System:**

* Now correctly installs cmark subdirectory

v0.3-2
====================

**Documentation:**

* New `output_section` command to render an output section heading in synopsis
* New comment section arguments for e.g. return value/description pairs or exception type documentation (see documentation for more info)
* New `see` comment section to list related entities using the comment sections arguments (see documentation as well)

**Library:**

* Improved blacklisting support
* Hidden name is now `*see-below*`
* Module index shows function signatures

**Tool:**

* Add `output.advanced_code_block` option to enable/disable new advanced code blocks that links to entities (enabled by default)
* Add `output.show_group_output_section` to enable implicit output sections for member groups (enabled by default)

v0.3-1
====================

**Documentation:**

* Now generates inline docs for member groups as well
* Now generates synopsis for undocumented entities again
* Hide template parameters in class names
* Add support for `\n` and `\t` in synopsis override
* Add support for documentation comments in macros
* Add `doc_` prefix to all documentation output files

**Library:**

* Allow multiple registration of entities (warning, not a critical error)
* Ignore `static_assert` silently
* Add support for friend function definitiones
* Add heuristic for include guard detection
* Improve libclang diagnostic output
* Various other bugfixes

**Tool:**

* Add `output.show_complex_noexcept` option that controls whether complex noexcept expressions will be shown in the synopsis
* Add `output.prefix` option that sets a global prefix for all output
* Improve standardese-config.cmake

v0.3
====================

**Documentation:**

* Add `synopsis` command to override the synopsis
* Add `group` command to group documentation together
* Add `module` command as a way to categorize entities
* Add template language
* Allow `unique_name` on files to override the output name
* Allow name lookup on entity links (when link starts with `*` or `?`)
* *Breaking:* Change hard line break character to a forward slash
* *Breaking:* Require `entity` and `file` command to be first in a comment
* *Breaking:* Remove template parameters for function template unique name
* Improve documentation headings
* Fix termination of section by all special commands
* Fix matching of end-of-line comments

**Library:**

* Parsing bugfixes
* Use clang as preprocessor instead of Boost.Wave
* Rewrite generation and synopsis to allow more advanced output

**Tool:**

* Add `output.inline_doc` option to enable inline documentation
* Add `output.show_modules` option to enable/disable showing the module of an entity in the output
* Add `output.show_macro_replacement` option to enable/disable showing the macro replacement in the synopsis
* Add `output.show_group_member_id` option to enable/disable showing an integral id for members of a member group
* Add `compilation.clang_binary` option to control clang binary used as preprocessor
* Add `compilation.ms_compatibility` option to give more control over MSVC compatibility as well as tweaked `compilation.ms_extensions`
* Add `template.*` options and other template support

v0.2-2
====================

**Documentation:**

* Simplify comment format: Now a special command can be at the beginning of each new line
* Remove section merging as it has become unnecessary
* Section is now active until paragraph end, another special command or hard line break

**Library:**

* Clarify AST vs semantic parent of `cpp_entity`
* Change preprocessing: Now the entire file is preprocessed before passing it to libclang
* Generate full synopsis for non-documented entities
* Bugfixes, bugfixes and bugfixes

**Tool:**

* Remove `comment.implicit_paragraph` option, it is obsolete
* Add `compilation.ms_extensions` option

v0.2-1
====================

**Build System:**

* Add pre-built binaries for Travis CI and Windows
* Improve Travis dependency management
* Improve Appveyor

v0.2
====================

**Build System:**

* New method of handling external projects

**Documentation:**

* Add ability to use almost arbitrary Markdown in the documentation comments

* Link to other entities via `[link-text](<> "unique-name")`, where `unique-name` is the name of the entity with all scopes and (template) parameters, `link-text` is the arbitrary text of the link and the literal `<>` denotes an empty URL.
 This can be abbreviated to `[unique-name]()` if you don't need a special text.
* New commmand: `exclude` to exclude entites from the output via comment

* New command: `unique_name` to override the unique name of an entity

* Support for more comment variants (`///`, `//!`, `/** ... */` and `/*! ... */`)

* Support for end-of line comments:
```cpp
int bar; //< Bar.
/// Bar continued.
```

* New commands: `file`, `param`, `tparam` and `base` to document the current file, (template) parameters and base classes.

* New command: `entity` to document an entity from a different location given its name.

**Library:**

* New `md_entity` hierachy

* Redid comment parsing (twice) and sectioning (twice)

* Redid outputting, new `output_format_base` hierachy, new `output` class merely acting as wrapper; `code_block_writer` now standalone class

* New functions in `cpp_entity`

* Internal changes

**Tool:**

* Multithreaded documentation generation and new option `jobs/j` to specify number of threads.

* Index generation

* More output formats set by the `output.format` option: CommonMark, HTML, Latex (experimental), Man (experimental) and XML

* New options:  `input.blacklist_dotfiles`, `comment.implicit_paragraph`, `comment.external_doc`, `output.link_extension`, `output.width` and `output.tab_width`

v0.1-1
====================

Bugfixes, better compiler support.

v0.1
====================

**Build System:**

* changed target names to reflect namespaces

* installation options

* support for libclang 3.8

* CMake integration for building with `standardese_generate()`

**Library::**

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

**Tool:**

* new options for compilation and entity filtering

* verbose output and coloring options

v0.0
====================

First basic prototype.
