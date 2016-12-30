# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# installs libclang on TravisCI (both MacOS and Ubuntu)
# usage: . travis_install_libclang.sh
# it exports the following variables:
# LLVM_DIR - top-level path
# LIBCLANG_LIBRARY - path to the libclang library
# LIBCLANG_INCLUDE_DIR - path to the libclang include directory# LIBCLANG_SYSTEM_INCLUDE_DIR - path to the clang system header files
# CLANG_BINARY - path to the clang++ binary
# it listens to:
# LLVM_VERSION - the version of LLVM to use, default is 3.9.1

if [ -z $LLVM_VERSION ]; then
    LLVM_VERSION="3.9.1"
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    wget --no-check-certificate http://llvm.org/releases/$LLVM_VERSION/clang+llvm-$LLVM_VERSION-x86_64-linux-gnu-ubuntu-14.04.tar.xz -O llvm-$LLVM_VERSION.xz

    tar -xJf llvm-$LLVM_VERSION.xz
    rm llvm-$LLVM_VERSION.xz
    mv clang+llvm-$LLVM_VERSION-x86_64-linux-gnu-ubuntu-14.04 llvm-$LLVM_VERSION

    export LLVM_DIR=$PWD/llvm-$LLVM_VERSION
    export LIBCLANG_LIBRARY=$LLVM_DIR/lib/libclang.so.${LLVM_VERSION%.*}
    export LIBCLANG_INCLUDE_DIR=$LLVM_DIR/include
    export LIBCLANG_SYSTEM_INCLUDE_DIR=$LLVM_DIR/lib/clang/$LLVM_VERSION/include
    export CLANG_BINARY=$LLVM_DIR/bin/clang++

    export LD_LIBRARY_PATH=$LLVM_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
    export PATH=$LLVM_DIR/bin${PATH:+:$PATH}

elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    wget --no-check-certificate http://llvm.org/releases/$LLVM_VERSION/clang+llvm-$LLVM_VERSION-x86_64-apple-darwin.tar.xz -O llvm-$LLVM_VERSION.xz

    tar -xJf llvm-$LLVM_VERSION.xz
    rm llvm-$LLVM_VERSION.xz
    mv clang+llvm-$LLVM_VERSION-x86_64-apple-darwin llvm-$LLVM_VERSION

    export LLVM_DIR=$PWD/llvm-$LLVM_VERSION
    export LIBCLANG_LIBRARY=$LLVM_DIR/lib/libclang.dylib
    export LIBCLANG_INCLUDE_DIR=$LLVM_DIR/include
    export LIBCLANG_SYSTEM_INCLUDE_DIR=$LLVM_DIR/lib/clang/$LLVM_VERSION/include
    export CLANG_BINARY=$LLVM_DIR/bin/clang++

    export DYLD_LIBRARY_PATH=$LLVM_DIR/lib${DYLD_LIBRARY_PATH:+:$DY_LD_LIBRARY_PATH}
    export PATH=$LLVM_DIR/bin${PATH:+:$PATH}
else
    echo "unknown Travis OS" >&2
    return 1
fi

echo "LLVM_DIR: $LLVM_DIR"
echo "LIBCLANG_LIBRARY: $LIBCLANG_LIBRARY"
echo "LIBCLANG_INCLUDE_DIR: $LIBCLANG_INCLUDE_DIR"
echo "LIBCLANG_SYSTEM_INCLUDE_DIR: $LIBCLANG_SYSTEM_INCLUDE_DIR"

