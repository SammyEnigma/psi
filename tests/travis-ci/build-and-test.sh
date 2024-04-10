#!/bin/sh

# Author:  Boris Pek
# Version: N/A
# License: Public Domain

set -e
set -x

if [ "${TARGET}" = "linux64" ]
then
    ./linux/build-in-ubuntu.sh
    cd builddir && sudo make install -j 1

    ls -alp /usr/bin/psi*
    ls -alp /usr/share/applications/psi*
    ls -alp /usr/share/pixmaps/psi*
    ls -alp /usr/share/psi*

    du -shc /usr/bin/psi*
    du -shc /usr/share/applications/psi*
    du -shc /usr/share/pixmaps/psi*
    du -shc /usr/share/psi*

    if [ -d "./plugins/generic" ]
    then
        ls -alp /usr/lib/psi*/plugins/*
        du -shc /usr/lib/psi*/plugins/*
    fi
elif [ "${TARGET}" = "macos64" ]
then
    ./mac/build-using-homebrew.sh

    ls -alp ../Psi*.dmg
    du -shc ../Psi*.dmg
elif [ "${TARGET}" = "windows64" ]
then
    ./win32/build-using-mxe.sh
else
    echo "Unknown target!"
    exit 1
fi
