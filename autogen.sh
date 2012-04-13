#!/bin/sh

missing() {
    echo The package $1 is not installed
}


ERROR=no

if ! which gtkdocize > /dev/null; then
    missing gtk-doc-tools
    ERROR=yes
fi

if ! which autoreconf > /dev/null; then
    missing autoconf
    ERROR=yes
fi

if ! pkg-config --exists gobject-introspection-1.0; then
    missing libgirepository1.0-dev
    ERROR=yes
fi

if ! pkg-config --exists gtk+-2.0; then
    missing libgtk2.0-dev
    ERROR=yes
fi


if test x$ERROR = xyes; then
    exit 1
fi


gtkdocize
autoreconf -i

if test -f configure; then
    ./configure --enable-gtk-doc $@
fi

