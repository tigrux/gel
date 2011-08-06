#!/bin/sh
if which gtkdocize; then
    gtkdocize
else
    echo You must install gtk-doc-tools
    exit 1
fi

if which autoreconf; then
  autoreconf -i
else
    echo You must install autoconf
    exit 1
fi

