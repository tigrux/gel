bin_PROGRAMS = gel

gel_CPPFLAGS = -Wall -Werror -ggdb
gel_CFLAGS = $(GOBJECT_CFLAGS) $(GI_CFLAGS) -I$(top_srcdir)/libgel
gel_LDFLAGS = $(GOBJECT_LIBS) $(GI_LIBS)
gel_LDADD = $(top_srcdir)/libgel/libgel.la

gel_SOURCES = gel.c

