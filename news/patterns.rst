**Added:**

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

* Sections do not treat entries of the form `\section key - text` specially
  anymore. This created a substantial amount of complexity during parsing and
  also only did not have full Markdown support in the `key` section. Instead,
  one should just use a proper Markdown list instead.

* Moved `section_type` from `standardese::markup` to the `standardese::comment`
  namespace where `command_type` and `inline_type` are already.

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

* Do not mention templating in README or command line help since it is not implemented.

* Check all return values from calls into cmark (by always going through an
  exception-throwing wrapper.)

* Simplified CommonMark extensions by not using any temporary nodes.
