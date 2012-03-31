noinst_PROGRAMS = gel

gel_CPPFLAGS = -Wall -ggdb
gel_CFLAGS = $(GTK_CFLAGS) -I$(top_srcdir)/libgel
gel_LDFLAGS = $(GTK_LIBS)
gel_LDADD = $(top_srcdir)/libgel/libgel.la

gel_SOURCES = gel.c

EXTRA_DIST = gel.vala test.gel test2.gel test3.gel test4.gel test5.gel
