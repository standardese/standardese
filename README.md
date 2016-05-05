# standardese

> Note: This is an early prototype and highly WIP.
> Many features are missing and there are probably bugs everywhere.

Standardese aims to be a nextgen [Doxygen](http://doxygen.org).
It consists of two parts: a library and a tool.

The library aims at becoming *the* documentation frontend that can be easily extended and customized.
It parses C++ code with the help of [libclang](http://clang.llvm.org/doxygen/group__CINDEX.html) and provides access to it.

The tool drives the library to generate documentation for user-specified files.
It currently only supports Markdown as an output format but might be extended in the future.

## Basic example

Consider the following C++ header file:

```cpp
#include <type_traits>

namespace std
{

    /// \effects Exchanges values stored in two locations.
    /// \requires Type `T` shall be `MoveConstructible` and `MoveAssignable`.
    template <class T>
    void swap(T &a, T &b) noexcept(is_nothrow_move_constructible<T>::value &&
                                    is_nothrow_move_assignable<T>::value);
}
```

This will generate the following documentation:

---

# Header file ``swap.cpp``


```cpp
#include <type_traits>

namespace std
{
    template <typename T>
    void swap(T & a, T & b) noexcept(is_nothrow_move_constructible<T>::value &&is_nothrow_move_assignable<T>::value);
}
```


## Function template ``swap<T>``


```cpp
template <typename T>
void swap(T & a, T & b) noexcept(is_nothrow_move_constructible<T>::value &&is_nothrow_move_assignable<T>::value);
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
Simply clone the project and run `cmake --build .` to build the library and the tool.

Both require libclang - only tested with version `3.7.1`.
If it isn't found, set the CMake variable `LIBCLANG_INCLUDE_DIR` to the folder where `clang-c/Index.h` is located,
`LIBCLANG_LIBRARY` to the library binary and `LIBCLANG_SYSTEM_INCLUDE_DIR` where e.g. `clang/3.7.1/include/cstddef` is.

The tool requires Boost.ProgramOptions and Boost.Filesystem, only tested with 1.60.

Once build simply run `./standardese --help` for usage.
