#!/bin/sh

MISSING="no"
DEB_MISSING=""
RPM_MISSING=""

if ! which automake > /dev/null 2>&1; then
    DEB_MISSING="$DEB_MISSING automake"
    RPM_MISSING="$RPM_MISSING automake"
    MISSING="yes"
fi

if ! which autoreconf > /dev/null 2>&1; then
    DEB_MISSING="$DEB_MISSING autoconf"
    RPM_MISSING="$RPM_MISSING autoconf"
    MISSING="yes"
fi

if ! which libtool > /dev/null 2>&1; then
    DEB_MISSING="$DEB_MISSING libtool"
    RPM_MISSING="$RPM_MISSING libtool"
    MISSING="yes"
fi

if ! which gtkdocize > /dev/null 2>&1; then
    DEB_MISSING="$DEB_MISSING gtk-doc-tools"
    RPM_MISSING="$RPM_MISSING gtk-doc"
    MISSING="yes"
fi

if which pkg-config > /dev/null 2>&1; then
    if ! pkg-config --exists gobject-introspection-1.0; then
        DEB_MISSING="$DEB_MISSING libgirepository1.0-dev"
        RPM_MISSING="$RPM_MISSING gobject-introspection-devel"
        MISSING="yes"
    fi
else
    DEB_MISSING="$DEB_MISSING pkg-config"
    RPM_MISSING="$RPM_MISSING pkg-config"
    MISSING="yes"
fi

if test "$MISSING" = "yes"; then
    echo "Some required packages are missing"
    if which yum > /dev/null 2>&1; then
        echo "If you are using redhat or fedora, install them with:"
        echo "sudo yum install$RPM_MISSING"
    elif which apt-get > /dev/null 2>&1; then
        echo "If you are using debian or ubuntu, install them with:"
        echo "sudo apt-get install$DEB_MISSING"
    fi
    exit 1
fi


gtkdocize
autoreconf -i

if test -f configure; then
    ./configure --enable-gtk-doc $@ && echo "Now type 'make' to compile"
fi

