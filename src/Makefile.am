AM_CFLAGS = -Wall -std=c99 -pedantic \
  -g -pg -finline

# Gnulib:
# includes generated files from top_builddir (not only top_srcdir)
AM_CPPFLAGS = -I$(top_srcdir)/gnulib/lib -I$(top_builddir)/gnulib/lib
LDADD = $(top_builddir)/gnulib/lib/libgnu.a
# _Not_ LIBADD...

# Woe icon and app infos
# Also check http://fragglet.livejournal.com/4448.html
.rc.o:
	$(WINDRES) -i $< -o $@

if HAVE_WINDRES
WOERES=woeres.rc
else
WOERES=
endif

bin_PROGRAMS = rapidiapo
