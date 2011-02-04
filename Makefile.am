if FLYN
# For local development use only.
include /etc/fctk/config.mk
endif

SUBDIRS = distro dry src
DIST_SUBDIRS = distro dry src

EXTRA_DIST = autogen.sh FAQ README.release-protocol

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dmapd.pc

if FLYN
# For local development use only.
dist-hook:
	make dry

rpm: dry dist
	$(BUILD_RPM)

deb: dry dist
	$(BUILD_DEB)

check: rpm deb
	$(CHECK)

publish: rpm deb
	$(PUBLISH)
endif
