if FLYN
# For local development use only.
include /etc/fctk/config.mk
endif

AUTOMAKE_OPTIONS = gnu

man_MANS = dmapd.1

EXTRA_DIST = dmapd.spec dmapd.1

if FLYN
# For local development use only.
$(PACKAGE).spec: $(PACKAGE).xml
	xsltproc --stringparam version $(VERSION) /usr/share/fctk/spec.xslt $(PACKAGE).xml > $@

$(PACKAGE).html: $(PACKAGE).xml
	xsltproc /usr/share/fctk/webpage.xslt $(PACKAGE).xml > $@

$(PACKAGE)_summary.html: $(PACKAGE).xml
	xsltproc /usr/share/fctk/html_summary.xslt $(PACKAGE).xml | tail -n 1 > $@

dmapd.1: dmapd.xml
	xsltproc /usr/share/fctk/manpage.xslt dmapd.xml > $@

README: $(PACKAGE).xml
	xsltproc /usr/share/fctk/README.xslt $(PACKAGE).xml > $@

usage_$(PACKAGE).c: $(PACKAGE).xml
	xsltproc /usr/share/fctk/usage.xslt > $@

control: $(PACKAGE).xml
	xsltproc /usr/share/fctk/control.xslt $(PACKAGE).xml > $@

all: $(PACKAGE).spec README $(PACKAGE).html $(PACKAGE)_summary.html $(PACKAGE).spec control usage_$(PACKAGE).c dmapd.1
	cp README ..
	cp control ../debian
	cp usage_$(PACKAGE).c ../src

endif
