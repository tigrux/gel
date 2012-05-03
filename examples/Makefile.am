noinst_PROGRAMS = test

test_CPPFLAGS = -Wall -Werror -ggdb
test_CFLAGS = $(GOBJECT_CFLAGS) $(GI_CFLAGS) -I$(top_srcdir)/libgel
test_LDFLAGS = $(GOBJECT_LIBS) $(GI_LIBS)
test_LDADD = $(top_srcdir)/libgel/libgel.la

test_SOURCES = test.c

EXTRA_DIST = test.vala \
    test.gel test2.gel test3.gel test4.gel \
    test5.gel test6.gel test7.gel test8.gel test9.gel
