# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# downloads standardese for Travis CI
# usage: STANDARDESE_TAG=tag . travis_get_standardese.sh
# requires that a recent libstdc++ and Boost 1.55 is installed (under Linux)
# will download libclang itself
# result is folder standardese in the current directory

wget https://raw.githubusercontent.com/foonathan/standardese/$STANDARDESE_TAG/travis_install_libclang.sh
. travis_install_libclang.sh
rm travis_install_libclang.sh

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    name=standardese-ubuntu-12.04.tar.gz
elif [ "$TRAVIS_OS_NAME" = "osx" ]; then
    name=standardese-osx.tar.gz
else
    echo "unknown Travis OS" >&2
    return 1
fi

wget https://github.com/foonathan/standardese/releases/download/$STANDARDESE_TAG/$name -O standardese.tar.gz
tar -xzf standardese.tar.gz
rm standardese.tar.gz

unset name

