# standardese

[![Build Status](https://travis-ci.org/foonathan/standardese.svg?branch=master)](https://travis-ci.org/foonathan/standardese)
[![Build status](https://ci.appveyor.com/api/projects/status/1aw8ml5lawu4mtyv/branch/master?svg=true)](https://ci.appveyor.com/project/foonathan/standardese/branch/master)
[![Join the chat at https://gitter.im/foonathan/standardese](https://badges.gitter.im/foonathan/standardese.svg)](https://gitter.im/foonathan/standardese?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

> Note: This is an early prototype and highly WIP.
> Many features are missing and there are probably bugs everywhere.

Standardese aims to be a nextgen [Doxygen](http://doxygen.org).
It consists of two parts: a library and a tool.

The library aims at becoming *the* documentation frontend that can be easily extended and customized.
It parses C++ code with the help of [libclang](http://clang.llvm.org/doxygen/group__CINDEX.html) and provides access to it.

The tool drives the library to generate documentation for user-specified files.
It currently only supports Markdown as an output format but might be extended in the future.

Read more in the introductory [blog post](http://foonathan.github.io/blog/2016/05/06/standardese-nextgen-doxygen.html).

## Basic example

Consider the following C++ header file:

```cpp
#include <type_traits>

namespace std
{

    /// \effects Exchanges values stored in two locations.
    ///
    /// \requires Type `T` shall be `MoveConstructible` and `MoveAssignable`.
    template <class T>
    void swap(T &a, T &b) noexcept(is_nothrow_move_constructible<T>::value &&
                                    is_nothrow_move_assignable<T>::value);
}
```

This will generate the following documentation:

# Header file `swap.cpp`


```cpp
#include <type_traits>

namespace std
{
    template <typename T>
    void swap(T & a, T & b) noexcept(is_nothrow_move_constructible<T>::value &&
    is_nothrow_move_assignable<T>::value);
}
```


## Function template ``swap<T>``


```cpp
template <typename T>
void swap(T & a, T & b) noexcept(is_nothrow_move_constructible<T>::value &&
is_nothrow_move_assignable<T>::value);
```


*Effects:* Exchanges values stored in two locations.

*Requires:* Type `T` shall be `MoveConstructible` and `MoveAssignable`.

---

The example makes it already clear:
Standardese aims to provide a documentation in a similar way to the C++ standard - hence the name.

This means that it provides commands to introduce so called *sections* in the documentation.
The current sections are all the C++ standard lists in `[structure.specifications]/3` like `\effects`, `\requires`, `\returns` and `\throws`.

For a more complete example check out [this gist](https://gist.github.com/foonathan/14e163b76804b6775d780eabcbaa6a51).

## Installation

Standardese uses [CMake](https://cmake.org/) as build system.
Simply clone the project and run `cmake -DSTANDARDESE_BUILD_TEST=OFF <source_dir>` followed by `cmake --build . --target install` to build the library and the tool and install it on your system.

Both require libclang - only tested with version `3.7.1` and `3.8`.
If it isn't found, set the CMake variable `LIBCLANG_INCLUDE_DIR` to the folder where `clang-c/Index.h` is located,
`LIBCLANG_LIBRARY` to the library binary and `LIBCLANG_SYSTEM_INCLUDE_DIR` where the system include files are located,
under a normal (Linux) installation it is `/usr/lib/clang/<version>/include`.

The library requires Boost.Wave (at least 1.55) and the tool requires Boost.ProgramOptions and Boost.Filesystem.
By default, Boost libraries are linked dynamically (except for Boost.ProgramOptions which is always linked statically),
but if you wish to link them statically, just add `-DBoost_USE_STATIC_LIBS=ON` to the cmake command.

Once built, simply run `standardese --help` for commandline usage.

### Windows

There is a pre-built binary for Windows 64 Bit, built with Appveyor and MSVC 14.
You need to install [libclang](http://llvm.org/releases/download.html) but should work out the box otherwise.

### Travis CI

There are pre-built binaries for Travis CI (both MacOS and Linux), useful for building documentation on your CI system.
Under Boost you are good to go, but Linux needs an update of libstdc++ and Boost 1.55:

```
addons:
  apt:
    sources: ['ubuntu-toolchain-r-test', 'boost-latest']
    packages: ['g++-5', 'libboost1.55-all-dev']<
```
For convenience you can use the script `travis_get_standardese.sh`.
It will download libclang and the `standardese` binary.
You can use it like so:

```
wget https://raw.githubusercontent.com/foonathan/standardese/travis_get_standardese.sh
STANDARDESE_TAG=tag-name . travis_get_standardese.sh
./standardese/standardese --version
```

## Documentation

> Disclaimer: Due to the lack of proper tooling there is currently no good documentation.
> If you need help or encounter a problem please contact me [on gitter](https://gitter.im/foonathan/standardese?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge), I'll usually answer within a day or faster.

### Basic commandline usage

The tool uses Boost.ProgramOptions for the options parsing, `--help` gives a good overview.

Basic usage is: `standardese [options] inputs`

The inputs can be both files or directories, in case of directory each file is documented, unless an input option is specified (see below).
The tool will currently generate a corresponding Markdown file with the documentation for each file it gets as input.

> Note: You only need and should give header files to standardese.
> The source files are unnecessary.

The options listed under "Generic options:" must be given to the commandline.
They include things like getting the version, enabling verbose output (please provide it for issues) or passing an additional configuration file.

The options listed under "Configuration" can be passed both to the commandline and to the config file.
They are subdivided into various sections:

* The `input.*` options are related to the inputs given to the tool.
They can be used to filter, both the files inside a directory and the entities in the source code.

* The `compilation.*` options are related to the compilation of the source.
You can pass macro definitions and include directories as well as a `commands_dir`.
This is a directory where a `compile_commands.json` file is located.
standardese will pass *all* the flags of all files to libclang.

> This has technical reasons because you give header files whereas the compile commands use only source files.

* The `comment.*` options are related to the syntax of the documentation markup.
You can set both the leading character and the name for each command, for example.

* The `output.*` options are related to the output generation.
It contains an option to set the human readable name of a section, for example.

The configuration file you can pass with `--config` uses an INI style syntax, e.g:

```
[compilation]
include_dir=foo/
macro_definition=FOO

[input]
blacklist_namespace=detail
extract_private=true
```

### Basic CMake usage

To ease the compilation options, you can call standardese from CMake like so:

```
find_package(standardese REQUIRED) # find standardese after installation

# generates a custom target that will run standardese to generate the documentation
standardese_generate(my_target CONFIG path/to/config_file
                     INCLUDE_DIRECTORY ${my_target_includedirs}
                     INPUT ${headers})
```

It will use a custom target that runs standardese.
You can specify the compilation options and inputs directly in CMake to allow shared variables.
All other options must be given in a config file.

See `standardese-config.cmake` for a documentation of `standardese_generate()`.

### Documentation syntax overview

standardese looks for documentation comments as shown in the following example:

```cpp
/// A regular C++ style documentation comment.
/// Multiple C++ style comments are merged automatically.
///   This line has *two* leading whitespaces because one is always skipped.

//! A C++ style comment using an exclamation mark.
/// It will also merge with other C++ style comments.
//! But don't worry, also with the exclamation mark styles.

/** A C style documentation commment. */
/** This is a different comment, they aren't merged.
 * But you can be fancy with the star at the beginning of the line.
 * It will ignore all whitespace, the star and the first following whitespace.
 */

/*! You can also use an exclamation mark. */
/// But neither will merge with any other comment.

int x; //< An end-of-line comment.
/// It will merge with C++ style comments.
int y; //< But this is a different end-of-line comment.
```

A comment corresponds to the entity on the line directly below or on the same line.
Inside the comment you can use arbitrary\* Markdown\* in the documentation comments and it will be rendered appropriately.

> The Markdown flavor used is [CommonMark](https://commonmark.org).
> standardese does not support inline HTML (for obvious reasons) or images.
> Inline HTML that isn't a raw HTML block will be treated as literal text.
> This allows writing `vector<T>` without markup or escaping in the comment, for example.

#### Linking

To link to an entity, use the syntax `[link-text](<> "unique-name")` (a CommonMark link with empty URL and a title of `unique-name`). If you don't want a special `link-text`, this can be shortened to `[unique-name]()` (a CommonMark link with empty URL and the name of an entity as text).
In either case `standardese` will insert the correct URL by searching for the entity with the given `unique-name`.

The `unique-name` of an entity is the name with all scopes, i.e. `foo::bar::baz`.

* For templates you need to append all parameters, i.e. `foo<A, B, C>`.

* For functions you need to append the signature (parameter types and cv and ref qualifier), i.e. `func()`, `bar(int,char)` or `type::foo() const &&`. If the signature is `()`, you can omit it.

* For (template) parameters it is of the form `function-unique-name.parameter-name`

* For base classes it is of the form `derived-class::base-class`

The `unique-name` doesn't care about whitespace, so `bar(const char*)`, `bar(const char *)` and `bar (constchar*)` are all the same.
Because it is sometimes long and ugly, you can override the unique name via the `unique_name` command (see below).

> For example you can override `bar(long, list, of, parameters)` to `bar()`.
> But keep in mind that it must be unique with regard to all overloads etc.
> Usually numbering would be a good choice, so `bar() (1)` or similar.

You can also use a short `unique-name` if there aren't multiple entities resolved to the same short name.
The short name is the `unique-name` but without a signature, i.e. for `foo<T>::func<U>(int) const`, the short name is `foo<T>::func<U>`.

You can also link to external documentations via the tool option `--comment.external_doc prefix=url`.
All `unique-name`s starting with `prefix` will be linked to the `url`.
If the `url` contains two dollar signs `$$`, they will be replaced by the `unique-name`.
By default the tool supports http://en.cppreference.com/w/ with a prefix of `std::` by default.

> You can override to a different URL by specifying `--comment.external_doc std::=new-url`.

#### Special commands

standardese adds its own sets of special commands.
A command is introduced by the *command character* (a backslash by default) at the beginning of a CommonMark text node, i.e. at the beginning of each line in the comment.

There are three kinds of special commands: *commands*, *sections* and *inlines*.

---

A *command* is used to control the documentation generation in some way.
A text that begins with a *command* doesn't appear in the output documentation at all.

There are the following *commands*:

* `exclude` - Manually excludes an entity from the documentation. It won't appear at all, not even in the synopsis.
It is as if the entity never existed in the first place.

* `unique_name {name}` - Overrides the unique name of an entity (e.g. for linking):
```cpp
/// Some documentation.
/// I can now link to `bar()` by writing [foo]().
///
/// \unique_name foo
void bar(int a, int c);
```

* `entity {unique-name}` - In a comment without a corresponding entity, names the entity to document:
```cpp
void foo();

/// This comment has no corresponding entity.
/// But the command specifies the entity it will belong to.
/// \entity foo
```
It also mixes with `unique_name` as you might expect.

* `file` - A shorthand for `\entity current-file-name`.

---

A *section* is the basic way of standardese documentation.
It supports all the sections the C++ standard uses, as explained in the example.
Those sections will create a paragraph in the output prefixed with a human readable name.
There are two special sections, `brief` and `details`.
They are not labeled in the output.

Unlike for a *command* text following a *section* is included in the output.
A *section* is active for the rest of the paragraph, a hard line break or until another special command is ecnountered.
Any `brief` sections will be merged together automatically.

If you don't specify a section for a paragraph, the first paragraph will be implictly `brief`, all others implictly `details`.

```cpp
/// \brief This text is brief.
///
/// This is implictly details.
/// \effects This is effects.
/// This is still effects.
/// \returns This is returns.\ 
/// Due to the hard break this is details again.
///
/// \notes This is notes.
/// \notes This is a different notes.
```

* Note: if the last character of any line in the source code - even comments - is a backslash,
the C preprocessor will merge it with the following line.
To prevent that, you need to put whitespace after the backslash.
CommonMark will still treat it as a hard line break, but the preprocessor won't. *

---

A *inline* is a special kind of command.
They are `param`, `tparam` and `base` and used to document (template) parameters and base classes,
because you cannot put a corresponding comment there.
As such they are shorthands for the `\entity unique-name` command.
They are followed by the name of entity they refer to.

> Technically, `param` and `tparam` are alias, so it doesn't matter which one you use.
> You must use `base` to refer to base classes, however,
> because template parameters and base classes can have the same name.

The *inline* and argument is stripped from the text.
The rest of the line will be treated as `brief` documentation.
Like a *section*, an *inline* ends when a new special command or hard line break is encountered or a paragraph ends.

> Note: You cannot use *sections* in *inlines*.

For example:

```cpp
/// Normal documentation for the function.
///
/// \param foo Brief documentation for the parameter `foo`.
/// It continues here with details.
/// \param bar
/// \exclude
///
/// The `\exclude` is part of the documentation for `bar`.
void func(int foo, int bar);
```

## Acknowledgements

Thanks a lot to:

* Manu @Manu343726 Sánchez, as always

* Jason @jpleau Pleau, for much feedback and requests on Gitter

* Mark @DarkerStar Gibbs, for feature suggestions

* Tristan @tcbrindle Brindle, for (better) clang support

* Marek @mkurdej Kurdej, for (better) MSVC support

* Victor @vitaut Zverovich, for bugfixes

* John @johnmcfarlane McFarlane, for issue reporting

And everyone else who shares and uses this project!
