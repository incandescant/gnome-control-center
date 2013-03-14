noinst_LTLIBRARIES = libconnection-editor.la

BUILT_SOURCES =					\
	net-connection-editor-resources.c	\
	net-connection-editor-resources.h

libconnection_editor_la_SOURCES = 		\
	$(BUILT_SOURCES)			\
	net-connection-editor.h			\
	net-connection-editor.c

libconnection_editor_la_CPPFLAGS = $(NETWORK_PANEL_CFLAGS)

libconnection_editor_la_LIBADD = $(NETWORK_PANEL_LIBS)

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/connection-editor.gresource.xml)
net-connection-editor-resources.c: connection-editor.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name net_connection_editor $<
net-connection-editor-resources.h: connection-editor.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name net_connection_editor $<

EXTRA_DIST = \
	$(resource_files) connection-editor.gresource.xml

-include $(top_srcdir)/git.mk
