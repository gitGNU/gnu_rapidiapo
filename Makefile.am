AUTOMAKE_OPTIONS = gnits
# Gnulib:
ACLOCAL_AMFLAGS = -I gnulib/m4

SUBDIRS = autopackage gnulib/lib src

# Copy those directly and recursively:
EXTRA_DIST = bootstrap tests

dist-hook:
	find $(distdir) -name '*~' | xargs -r rm -f
	make -C $(distdir)/autopackage -f Makefile.autopackage
