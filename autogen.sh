#!/bin/sh
if which gtkdocize; then
    gtkdocize
else
    echo The package gtk-doc-tools is not installed
    exit 1
fi

if which autoreconf; then
    autoreconf -i
else
    echo The package autoreconf is not install
    exit 1
fi

if test -f configure; then
    ./configure $@
fi
