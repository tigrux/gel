noinst_PROGRAMS = test

test_CPPFLAGS = -Wall -Werror -ggdb
test_CFLAGS = $(GI_CFLAGS) -I$(top_srcdir)/libgel
test_LDFLAGS = $(GI_LIBS)
test_LDADD = $(top_srcdir)/libgel/libgel.la

test_SOURCES = test.c

EXTRA_DIST = test.vala test.gel test2.gel test3.gel test4.gel test5.gel
