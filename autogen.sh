#!/bin/sh

MISSING=""

if ! which gtkdocize > /dev/null; then
    MISSING="$MISSING gtk-doc-tools"
fi

if ! which autoreconf > /dev/null; then
    MISSING="$MISSING autoconf"
fi

if ! which automake > /dev/null; then
    MISSING="$MISSING automake"
fi

if ! which libtool > /dev/null; then
    MISSING="$MISSING libtool"
fi

if which pkg-config > /dev/null; then
    if ! pkg-config --exists gobject-introspection-1.0; then
        MISSING="$MISSING libgirepository1.0-dev"
    fi
else
    MISSING="$MISSING pkg-config"
fi

if test -n "$MISSING"; then
    echo "Some required packages are missing"
    echo "If you are using debian or ubuntu, install them with:"
    echo "sudo apt-get install $MISSING"
    exit 1
fi


gtkdocize
autoreconf -i

if test -f configure; then
    ./configure --enable-gtk-doc $@ && echo "Now type 'make' to compile"
fi

