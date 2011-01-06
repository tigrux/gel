lib_LTLIBRARIES = libgel.la
lib_VERSION = 0:1:0

libgel_la_CPPFLAGS = -Wall -Werror -ggdb
libgel_la_CFLAGS = $(GOBJECT_CFLAGS)
libgel_la_LIBS = $(GOBJECT_LIBS)
libgel_la_LDFLAGS = -module -export-dynamic -version-info $(lib_VERSION)

libgel_la_SOURCES = \
	gelclosure.c \
	gelcontext.c \
	geldebug.c \
	gelparser.c \
	gelsymbols.c \
	gelvalue.c

libgel_includedir = $(includedir)/gel-0.1
libgel_include_HEADERS = \
	gel.h \
	gelcontext.h \
	gelparser.h \
	gelvalue.h

