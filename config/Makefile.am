MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = default standard illume mobile desktop

EET_EET = @eet_eet@

EXTRA_DIST = profile.src

filesdir = $(datadir)/elementary/config

files_DATA = profile.cfg

%.cfg: %.src
	$(EET_EET) -i \
	$(top_builddir)/config/$@ config \
	$< 0

clean-local:
	rm -rf *.cfg *~
