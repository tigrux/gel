lib_LTLIBRARIES = libgel.la
lib_VERSION = 0:1:0

libgel_la_CPPFLAGS = -Wall -Werror -ggdb

if HAVE_GOBJECT_INTROSPECTION
    libgel_la_CFLAGS = $(GI_CFLAGS)
    libgel_la_LIBS = $(GI_LIBS)
else
    libgel_la_CFLAGS = $(GOBJECT_CFLAGS)
    libgel_la_LIBS = $(GOBJECT_LIBS)
endif

libgel_la_LDFLAGS = -module -export-dynamic -version-info $(lib_VERSION)

libgel_la_SOURCES = \
	gelclosure.c \
	gelcontext.c \
	gelcontextparams.c \
	gelerrors.c \
	gelparser.c \
	gelpredefined.c \
	gelvalue.c \
	gelsymbol.c \
	gelvariable.c \
	gelmacro.c

if HAVE_GOBJECT_INTROSPECTION
    libgel_la_SOURCES += geltypeinfo.c geltypelib.c
endif

libgel_includedir = $(includedir)/gel-0.1
libgel_include_HEADERS = \
	gel.h \
	gelcontext.h \
	gelparser.h \
	gelvalue.h \
	gelclosure.h \
	gelarray.h

noinst_HEADERS = \
	gelcontextprivate.h \
	gelvalueprivate.h \
	gelclosureprivate.h \
	gelsymbol.h \
	gelerrors.h \
	gelvariable.h \
	gelmacro.h

if HAVE_GOBJECT_INTROSPECTION
    noinst_HEADERS += geltypeinfo.h geltypelib.h
endif

