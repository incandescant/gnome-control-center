INCLUDES =					\
	-I$(top_srcdir)				\
	$(SHELL_CFLAGS)				\
	$(CHEESE_CFLAGS)			\
	-I$(top_srcdir)/panels/common/		\
	-I$(top_srcdir)/libgd

all-local: check-local

noinst_LTLIBRARIES = libshell.la

libshell_la_SOURCES = \
	cc-shell-model.c			\
	cc-shell-model.h

bin_PROGRAMS = gnome-control-center

gnome_control_center_SOURCES =			\
	$(BUILT_SOURCES)			\
	main.c					\
	cc-application.c			\
	cc-application.h			\
	cc-shell-log.c				\
	cc-shell-log.h				\
	cc-window.c				\
	cc-window.h				\
	cc-shell-category-view.c		\
	cc-shell-category-view.h		\
	cc-shell-item-view.c			\
	cc-shell-item-view.h			\
	cc-editable-entry.c			\
	cc-editable-entry.h			\
	cc-panel-loader.c			\
	cc-panel-loader.h			\
	cc-panel.c				\
	cc-panel.h				\
	cc-shell.c				\
	cc-shell.h				\
	hostname-helper.c			\
	hostname-helper.h			\
	cc-hostname-entry.c			\
	cc-hostname-entry.h			\
	$(MARSHAL_FILES)

gnome_control_center_LDFLAGS = -export-dynamic

gnome_control_center_LDADD =						\
	libshell.la							\
	$(SHELL_LIBS)							\
	$(CHEESE_LIBS)							\
	$(top_builddir)/libgd/libgd.la					\
	$(top_builddir)/panels/common/liblanguage.la			\
	$(top_builddir)/panels/background/libbackground.la		\
	$(top_builddir)/panels/color/libcolor.la			\
	$(top_builddir)/panels/datetime/libdate_time.la			\
	$(top_builddir)/panels/display/libdisplay.la			\
	$(top_builddir)/panels/info/libinfo.la				\
	$(top_builddir)/panels/keyboard/libkeyboard.la			\
	$(top_builddir)/panels/mouse/libmouse-properties.la		\
	$(top_builddir)/panels/notifications/libnotifications.la	\
	$(top_builddir)/panels/online-accounts/libonline-accounts.la	\
	$(top_builddir)/panels/power/libpower.la			\
	$(top_builddir)/panels/privacy/libprivacy.la			\
	$(top_builddir)/panels/region/libregion.la			\
	$(top_builddir)/panels/search/libsearch.la			\
	$(top_builddir)/panels/sharing/libsharing.la			\
	$(top_builddir)/panels/sound/libsound.la			\
	$(top_builddir)/panels/universal-access/libuniversal-access.la	\
	$(top_builddir)/panels/user-accounts/libuser-accounts.la

if BUILD_WACOM
gnome_control_center_LDADD += $(top_builddir)/panels/wacom/libwacom-properties.la
endif

if BUILD_PRINTERS
gnome_control_center_LDADD += $(top_builddir)/panels/printers/libprinters.la
endif

if BUILD_NETWORK
gnome_control_center_LDADD += $(top_builddir)/panels/connman/libnetwork.la
endif

if BUILD_BLUETOOTH
gnome_control_center_LDADD += $(top_builddir)/panels/bluetooth/libbluetooth.la
endif

AM_CPPFLAGS = -DGNOMELOCALEDIR="\"$(datadir)/locale\""

sysdir = $(datadir)/applications
sys_in_files = gnome-control-center.desktop.in
sys_DATA = $(sys_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

completiondir = $(datadir)/bash-completion/completions
completion_in_files = completions/gnome-control-center.in
completion_DATA = $(completion_in_files:.in=)
completions/gnome-control-center: completions/gnome-control-center.in list-panel.sh
	$(AM_V_GEN) mkdir -p `dirname $@` && cat $< | sed "s,@PANELS@,`$(srcdir)/list-panel.sh $(top_srcdir)`," > $@

EXTRA_DIST =					\
	gnome-control-center.desktop.in.in	\
	$(completion_in_files)			\
	list-panel.sh

CLEANFILES = $(BUILT_SOURCES) $(completion_DATA)
DISTCLEANFILES = gnome-control-center.desktop gnome-control-center.desktop.in

noinst_PROGRAMS = test-hostname
test_hostname_SOURCES = hostname-helper.c hostname-helper.h test-hostname.c
test_hostname_LDADD = $(PANEL_LIBS) $(INFO_PANEL_LIBS)
test_hostname_CFLAGS = $(INCLUDES)

EXTRA_DIST += hostnames-test.txt
check-local: test-hostname
	$(builddir)/test-hostname $(srcdir)/hostnames-test.txt > /dev/null

-include $(top_srcdir)/git.mk
