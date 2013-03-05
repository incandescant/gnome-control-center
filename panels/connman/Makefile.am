SUBDIRS = icons

cappletname = network

INCLUDES = 						\
	$(PANEL_CFLAGS)					\
	$(NETWORK_PANEL_CFLAGS)				\
	-DGNOMELOCALEDIR="\"$(datadir)/locale\""	\
	$(NULL)

noinst_LTLIBRARIES = libnetwork.la

BUILT_SOURCES =			\
	cc-network-resources.c	\
	cc-network-resources.h

dbus_built_sources =	manager.c manager.h \
			technology.c technology.h

manager.c: manager.h
manager.h: Makefile.am manager-interface.xml
	gdbus-codegen 						\
		--interface-prefix net.connman.			\
		--generate-c-code manager 			\
		$(srcdir)/manager-interface.xml

technology.c: technology.h
technology.h: Makefile.am technology-interface.xml
	gdbus-codegen 						\
		--interface-prefix net.connman.			\
		--generate-c-code technology 			\
		$(srcdir)/technology-interface.xml

libnetwork_la_SOURCES =					\
	$(dbus_built_sources)				\
	$(BUILT_SOURCES)				\
	cc-network-panel.c				\
	cc-network-panel.h

libnetwork_la_LIBADD = $(PANEL_LIBS)

libnetwork_la_LDFLAGS = $(PANEL_LDFLAGS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/network.gresource.xml)
cc-network-resources.c: network.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name cc_network $<
cc-network-resources.h: network.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name cc_network $<

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = gnome-network-panel.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = $(desktop_in_files) $(desktop_DATA) $(BUILT_SOURCES) $(dbus_built_sources)
EXTRA_DIST = $(resource_files) network.gresource.xml manager-interface.xml technology-interface.xml

-include $(top_srcdir)/git.mk
