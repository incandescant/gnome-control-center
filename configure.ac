m4_define([gnome_control_center_version], 3.8.0)
AC_INIT([gnome-control-center], [gnome_control_center_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gnome-control-center])

AC_CONFIG_SRCDIR([shell])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz tar-ustar check-news])
AM_MAINTAINER_MODE([enable])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

# Internationalization support

IT_PROG_INTLTOOL([0.40.1])

GETTEXT_PACKAGE=gnome-control-center-2.0
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package])

GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])

AC_PATH_XTRA
x_libs="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"

AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

AC_ARG_ENABLE(documentation,
              AC_HELP_STRING([--enable-documentation],
                             [build documentation]),,
              enable_documentation=yes)
if test x$enable_documentation = xyes; then
   AC_PATH_PROG([XSLTPROC], [xsltproc])
   if test x$XSLTPROC = x; then
      AC_MSG_ERROR([xsltproc is required to build documentation])
   fi
fi
AM_CONDITIONAL(BUILD_DOCUMENTATION, test x$enable_documentation = xyes)

dnl Region panel
savecppflags=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $X_CFLAGS"
AC_CHECK_HEADERS([X11/Xlib.h])
AC_CHECK_LIB(Xxf86misc, XF86MiscQueryExtension, [
  AC_CHECK_HEADERS([X11/extensions/xf86misc.h], [XF86MISC_LIBS="-lXxf86misc"],[],
[#if HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
])])
AC_SUBST(XF86MISC_LIBS)
AC_CHECK_HEADERS(X11/extensions/XKB.h)
CPPFLAGS=$savecppflags

AC_CHECK_LIB(m, floor)

# IBus support
IBUS_REQUIRED_VERSION=1.4.99

AC_ARG_ENABLE(ibus,
        AS_HELP_STRING([--disable-ibus],
                       [Disable IBus support]),
        enable_ibus=$enableval,
        enable_ibus=yes)

if test "x$enable_ibus" = "xyes" ; then
        IBUS_MODULE="ibus-1.0 >= $IBUS_REQUIRED_VERSION"
        AC_DEFINE(HAVE_IBUS, 1, [Defined if IBus support is enabled])
else
        IBUS_MODULE=
fi

# EggListBox submodule
prev_top_build_prefix=$ac_top_build_prefix
prev_ac_configure_args=$ac_configure_args
AX_CONFIG_DIR([egg-list-box])
ac_top_build_prefix=$prev_top_build_prefix
ac_configure_args=$prev_ac_configure_args

export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:"$ac_top_build_prefix"egg-list-box

dnl ==============================================
dnl Check that we meet the  dependencies
dnl ==============================================

GLIB_REQUIRED_VERSION=2.35.1
GTK_REQUIRED_VERSION=3.7.7
PA_REQUIRED_VERSION=2.0
CANBERRA_REQUIRED_VERSION=0.13
GDKPIXBUF_REQUIRED_VERSION=2.23.0
POLKIT_REQUIRED_VERSION=0.103
GSD_REQUIRED_VERSION=3.7.3
LIBNOTIFY_REQUIRED_VERSION=0.7.3
GNOME_DESKTOP_REQUIRED_VERSION=3.7.5
SCHEMAS_REQUIRED_VERSION=3.7.2.2
LIBWACOM_REQUIRED_VERSION=0.7
CLUTTER_REQUIRED_VERSION=1.11.3
GOA_REQUIRED_VERSION=3.5.90
ACCOUNTSSERVICE_REQUIRED_VERSION=0.6.30
COLORD_REQUIRED_VERSION=0.1.29
COLORD_GTK_REQUIRED_VERSION=0.1.24
CONNMAN_REQUIRED_VERSION=1.11

COMMON_MODULES="gtk+-3.0 >= $GTK_REQUIRED_VERSION
 glib-2.0 >= $GLIB_REQUIRED_VERSION
 gthread-2.0
 gio-2.0
 gio-unix-2.0
 gsettings-desktop-schemas >= $SCHEMAS_REQUIRED_VERSION
 libnotify >= $LIBNOTIFY_REQUIRED_VERSION"

LIBGD_INIT([_view-common notification main-toolbar header-bar stack static])

PKG_CHECK_MODULES(LIBLANGUAGE, $COMMON_MODULES gnome-desktop-3.0 fontconfig)
PKG_CHECK_MODULES(LIBSHORTCUTS, $COMMON_MODULES x11)
# egg-list-box is a static library, so it must be shared among all panels
# or it breaks GType registration
PKG_CHECK_MODULES(SHELL, $COMMON_MODULES x11 egg-list-box polkit-gobject-1 >= $POLKIT_REQUIRED_VERSION)
PKG_CHECK_MODULES(BACKGROUND_PANEL, $COMMON_MODULES libxml-2.0 gnome-desktop-3.0
                  gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED_VERSION)
PKG_CHECK_MODULES(DATETIME_PANEL, $COMMON_MODULES
                  gnome-desktop-3.0 >= $GNOME_DESKTOP_REQUIRED_VERSION
                  polkit-gobject-1 >= $POLKIT_REQUIRED_VERSION
                  gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED_VERSION)
PKG_CHECK_MODULES(DISPLAY_PANEL, $COMMON_MODULES gnome-desktop-3.0 >= 3.1.0)
PKG_CHECK_MODULES(INFO_PANEL, $COMMON_MODULES libgtop-2.0 gl
		  polkit-gobject-1 >= $POLKIT_REQUIRED_VERSION)
PKG_CHECK_MODULES(KEYBOARD_PANEL, $COMMON_MODULES
                  gnome-desktop-3.0 >= $GNOME_DESKTOP_REQUIRED_VERSION
                  x11)
PKG_CHECK_MODULES(MEDIA_PANEL, $COMMON_MODULES)
PKG_CHECK_MODULES(MOUSE_PANEL, $COMMON_MODULES xi >= 1.2
                  gnome-settings-daemon >= $GSD_REQUIRED_VERSION x11)
PKG_CHECK_MODULES(NETWORK_PANEL, $COMMON_MODULES gmodule-2.0)
PKG_CHECK_MODULES(NOTIFICATIONS_PANEL, $COMMON_MODULES libgnome-menu-3.0)
PKG_CHECK_MODULES(ONLINE_ACCOUNTS_PANEL, $COMMON_MODULES goa-1.0 goa-backend-1.0 >= $GOA_REQUIRED_VERSION)
PKG_CHECK_MODULES(POWER_PANEL, $COMMON_MODULES upower-glib >= 0.9.1
                  gnome-settings-daemon >= $GSD_REQUIRED_VERSION)
PKG_CHECK_MODULES(COLOR_PANEL, $COMMON_MODULES
                  colord >= $COLORD_REQUIRED_VERSION
                  colord-gtk >= $COLORD_GTK_REQUIRED_VERSION
                  gnome-desktop-3.0 >= $GNOME_DESKTOP_REQUIRED_VERSION)
PKG_CHECK_MODULES(PRINTERS_PANEL, $COMMON_MODULES
                  polkit-gobject-1 >= $POLKIT_REQUIRED_VERSION)
PKG_CHECK_MODULES(PRIVACY_PANEL, $COMMON_MODULES)
PKG_CHECK_MODULES(REGION_PANEL, $COMMON_MODULES
                  polkit-gobject-1 >= $POLKIT_REQUIRED_VERSION
                  gnome-desktop-3.0 >= $GNOME_DESKTOP_REQUIRED_VERSION
                  accountsservice >= $ACCOUNTSSERVICE_REQUIRED_VERSION
                  $IBUS_MODULE)
PKG_CHECK_MODULES(SCREEN_PANEL, $COMMON_MODULES)
PKG_CHECK_MODULES(SEARCH_PANEL, $COMMON_MODULES)
PKG_CHECK_MODULES(SOUND_PANEL, $COMMON_MODULES libxml-2.0
                  libcanberra-gtk3 >= $CANBERRA_REQUIRED_VERSION
                  libpulse >= $PA_REQUIRED_VERSION
                  libpulse-mainloop-glib >= $PA_REQUIRED_VERSION)
PKG_CHECK_MODULES(UNIVERSAL_ACCESS_PANEL, $COMMON_MODULES)
PKG_CHECK_MODULES(USER_ACCOUNTS_PANEL, $COMMON_MODULES
                  polkit-gobject-1 >= $POLKIT_REQUIRED_VERSION
                  gnome-desktop-3.0
                  gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED_VERSION
                  pwquality
                  accountsservice >= $ACCOUNTSSERVICE_REQUIRED_VERSION)
PKG_CHECK_MODULES(SHARING_PANEL, $COMMON_MODULES egg-list-box)
PKG_CHECK_MODULES(REMOTE_LOGIN_HELPER, glib-2.0 >= $GLIB_REQUIRED_VERSION gio-2.0)

PKG_CHECK_MODULES(GVC, gobject-2.0 libpulse libpulse-mainloop-glib)
AM_CONDITIONAL(HAVE_INTROSPECTION, false)

GDESKTOP_PREFIX=`$PKG_CONFIG --variable prefix gsettings-desktop-schemas`
AC_SUBST(GDESKTOP_PREFIX)

# Check for ConnMan
PKG_CHECK_MODULES(CONNMAN, connman >= $CONNMAN_REQUIRED_VERSION,
                  [have_connman=yes], have_connman=no)
if test "x$have_connman" = xno ; then
        AC_MSG_WARN(*** Network panel will not be built (ConnMan not found) ***)
else
        AC_DEFINE(BUILD_NETWORK, 1, [Define to 1 to build the Network panel])
fi
AM_CONDITIONAL(BUILD_NETWORK, [test x$have_connman = xyes])
if test x${have_connman} = xyes; then
  AC_DEFINE(HAVE_CONNMAN, 1, [Define to 1 if ConnMan is available])
fi

# Check for libmm-glib for ModemManager1 support
PKG_CHECK_MODULES(MM_GLIB, mm-glib,
                  [have_libmm_glib=yes], have_libmm_glib=no)
if test "x$have_libmm_glib" = xno ; then
  AC_MSG_WARN(*** Network panel will not be built with ModemManager1 support (libmm-glib not found) ***)
else
  AC_DEFINE(HAVE_MM_GLIB, 1, [Defined if libmm-glib is available])
fi
AM_CONDITIONAL(HAVE_MM_GLIB, test "x$have_libmm_glib" = "xyes")

# Check for gnome-bluetooth
PKG_CHECK_MODULES(BLUETOOTH, $COMMON_MODULES gnome-bluetooth-1.0 >= 3.5.5,
		  [have_bluetooth=yes], have_bluetooth=no)
if test "x$have_bluetooth" = xyes ; then
	AC_DEFINE(BUILD_BLUETOOTH, 1, [Define to 1 to build the Bluetooth panel])
fi
AM_CONDITIONAL(BUILD_BLUETOOTH, [test x$have_bluetooth = xyes])
if test x${have_bluetooth} = xyes; then
  AC_DEFINE(HAVE_BLUETOOTH, 1, [Define to 1 if bluetooth support is available])
fi

# Check for smbclient
PKG_CHECK_MODULES(SMBCLIENT, smbclient, [have_smbclient=yes], [have_smbclient=no])

if test x"$have_smbclient" = x"no"; then
    AC_MSG_WARN(*** Printer panel will not be built (smbclient.pc not found) ***)
    enable_cups=no
fi

# Check for CUPS 1.4 or newer
AC_ARG_ENABLE([cups],
              AS_HELP_STRING([--disable-cups], [disable CUPS support (default: enabled)]),,
              [enable_cups=yes])

if test x"$enable_cups" != x"no" ; then
  AC_PROG_SED

  AC_PATH_PROG(CUPS_CONFIG, cups-config)

  if test x$CUPS_CONFIG = x; then
    AC_MSG_ERROR([cups-config not found but CUPS support requested])
  fi

  CUPS_API_VERSION=`$CUPS_CONFIG --api-version`
  CUPS_API_MAJOR=`echo $ECHO_N $CUPS_API_VERSION | cut -d . -f 1`
  CUPS_API_MINOR=`echo $ECHO_N $CUPS_API_VERSION | cut -d . -f 2`

  AC_CHECK_HEADERS([cups/cups.h cups/http.h cups/ipp.h cups/ppd.h],,
                   AC_MSG_ERROR([CUPS headers not found but CUPS support requested]))

  if ! test $CUPS_API_MAJOR -gt 1 -o \
            $CUPS_API_MAJOR -eq 1 -a $CUPS_API_MINOR -ge 4 ; then
    AC_MSG_ERROR([CUPS 1.4 or newer not found, but CUPS support requested])
  fi

  AC_DEFINE(BUILD_PRINTERS, 1, [Define to 1 to build the Printers panel])

  CUPS_CFLAGS=`$CUPS_CONFIG --cflags | $SED -e 's/-O\w*//g' -e 's/-m\w*//g'`
  CUPS_LIBS=`$CUPS_CONFIG --libs`
  AC_SUBST(CUPS_CFLAGS)
  AC_SUBST(CUPS_LIBS)
fi

AM_CONDITIONAL(BUILD_PRINTERS, [test x"$enable_cups" = x"yes"])

# Optional dependency for the user accounts panel
AC_ARG_WITH([cheese],
            AS_HELP_STRING([--with-cheese], [enable cheese webcam support]),,
            with_cheese=auto)

if test x"$with_cheese" != x"no" ; then
   PKG_CHECK_MODULES(CHEESE, gstreamer-1.0 cheese-gtk >= 3.5.91 cheese clutter-gtk-1.0, [have_cheese=yes], [have_cheese=no])
   if test x${have_cheese} = xyes; then
     AC_DEFINE(HAVE_CHEESE, 1, [Define to 1 to enable cheese webcam support])
   fi
   if test x${with_cheese} = xyes && test x${have_cheese} = xno; then
     AC_MSG_ERROR([Cheese configured but not found])
   fi
else
   have_cheese=no
fi
AM_CONDITIONAL(BUILD_CHEESE, test x${have_cheese} = xyes)

# wacom is disabled for s390/s390x and non Linux platforms (needs udev)
case $host_os in
  linux*)
    if test "$host_cpu" = s390 -o "$host_cpu" = s390x; then
      have_wacom=no
    else
      PKG_CHECK_MODULES(WACOM_PANEL, $COMMON_MODULES
                        gnome-settings-daemon >= $GSD_REQUIRED_VERSION
                        xi >= 1.2 x11 libwacom >= $LIBWACOM_REQUIRED_VERSION
                        gnome-desktop-3.0 >= $GNOME_DESKTOP_REQUIRED_VERSION)
      AC_DEFINE(BUILD_WACOM, 1, [Define to 1 to build the Wacom panel])
      have_wacom=yes
    fi
    ;;
  *)
    have_wacom=no
    ;;
esac
AM_CONDITIONAL(BUILD_WACOM, [test x"$have_wacom" = x"yes"])

# Kerberos kerberos support
AC_PATH_PROG(KRB5_CONFIG, krb5-config, no)
if test "$KRB5_CONFIG" = "no"; then
  AC_MSG_ERROR([krb5-config executable not found in your path - should be installed with the kerberos libraries])
fi

AC_MSG_CHECKING(for krb5 libraries and flags)
KRB5_CFLAGS="`$KRB5_CONFIG --cflags`"
KRB5_LIBS="`$KRB5_CONFIG --libs`"
AC_MSG_RESULT($KRB5_CFLAGS $KRB5_LIBS)

AC_SUBST(KRB5_CFLAGS)
AC_SUBST(KRB5_LIBS)

USER_ACCOUNTS_PANEL_CFLAGS="$USER_ACCOUNTS_PANEL_CFLAGS $KRB5_CFLAGS"
USER_ACCOUNTS_PANEL_LIBS="$USER_ACCOUNTS_PANEL_LIBS $KRB5_LIBS"

dnl =======================================
dnl Panels
dnl =======================================

PANEL_CFLAGS="-I\$(top_srcdir)/ -DG_LOG_DOMAIN=\"\\\"\$(cappletname)-cc-panel\\\"\" -DPANEL_ID=\"\\\"\$(cappletname)\\\"\""
AC_SUBST(PANEL_CFLAGS)

PANEL_LIBS=""
AC_SUBST(PANEL_LIBS)

dnl ==============================================
dnl libsocialweb
dnl ==============================================

AC_MSG_CHECKING([Enable libsocialweb support])
AC_ARG_WITH([libsocialweb],
   AS_HELP_STRING([--with-libsocialweb],
                  [enable libsocialweb support]),,
   [with_libsocialweb=no])
AC_MSG_RESULT([$with_libsocialweb])

if test "x$with_libsocialweb" == "xyes"; then
  PKG_CHECK_MODULES(SOCIALWEB, libsocialweb-client)
  AC_DEFINE(HAVE_LIBSOCIALWEB, 1, [Defined if libsocialweb is available])
fi
AM_CONDITIONAL(WITH_LIBSOCIALWEB, test "x$with_libsocialweb" = "xyes")


dnl =======================================
dnl Update Mime Database
dnl =======================================

AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)

AC_ARG_ENABLE(update-mimedb,
   AS_HELP_STRING([--disable-update-mimedb],
                  [do not update mime database after installation]),,
                   enable_update_mimedb=yes)
AM_CONDITIONAL(ENABLE_UPDATE_MIMEDB, test x$enable_update_mimedb = xyes)

CONTROL_CENTER_VERSION=gnome_control_center_version
AC_SUBST(CONTROL_CENTER_VERSION)

dnl =======================================
dnl Finish
dnl =======================================

# Turn on the additional warnings last

AC_ARG_ENABLE(more-warnings,
              AS_HELP_STRING([--enable-more-warnings],
                             [Maximum compiler warnings]),
              set_more_warnings="$enableval",[
	      if test -d $srcdir/.git; then
	        set_more_warnings=yes
	      else
	        set_more_warnings=no
              fi])

AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
        AC_MSG_RESULT(yes)
        CFLAGS="\
        -Wall -Wclobbered -Wempty-body -Wignored-qualifiers \
        -Wmissing-field-initializers -Wmissing-parameter-type \
        -Wold-style-declaration -Woverride-init -Wtype-limits \
        -Wuninitialized \
        -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wnested-externs -Wpointer-arith \
        -Wcast-align -Wsign-compare \
        $CFLAGS"

	# Only add this when optimizing is enabled (default)
	AC_MSG_CHECKING([whether optimization is enabled])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#if __OPTIMIZE__ == 0
	                                     #error No optimization
	                                     #endif
	                                   ]], [[]])],
	                  [has_optimization=yes],
	                  [has_optimization=no])
	if test $has_optimization = yes; then
		CFLAGS="$CFLAGS -Wp,-D_FORTIFY_SOURCE=2"
	fi
	AC_MSG_RESULT($has_optimization)

        for option in -Wno-strict-aliasing -Wno-sign-compare; do
                SAVE_CFLAGS="$CFLAGS"
                CFLAGS="$CFLAGS $option"
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
                                  [has_option=yes],
                                  [has_option=no])
                if test $has_option = no; then
                        CFLAGS="$SAVE_CFLAGS"
                fi
                AC_MSG_RESULT($has_option)
                unset has_option
                unset SAVE_CFLAGS
        done
        unset option
else
        AC_MSG_RESULT(no)
fi


AC_CONFIG_FILES([
Makefile
panels/Makefile
panels/common/Makefile
panels/background/Makefile
panels/background/gnome-background-panel.desktop.in
panels/bluetooth/Makefile
panels/bluetooth/gnome-bluetooth-panel.desktop.in
panels/datetime/Makefile
panels/datetime/gnome-datetime-panel.desktop.in
panels/datetime/po-timezones/Makefile
panels/display/Makefile
panels/display/gnome-display-panel.desktop.in
panels/keyboard/Makefile
panels/keyboard/gnome-keyboard-panel.desktop.in
panels/keyboard/gnome-keybindings.pc
panels/region/Makefile
panels/region/gnome-region-panel.desktop.in
panels/mouse/Makefile
panels/mouse/gnome-mouse-panel.desktop.in
panels/online-accounts/Makefile
panels/online-accounts/gnome-online-accounts-panel.desktop.in
panels/online-accounts/icons/Makefile
panels/online-accounts/icons/16x16/Makefile
panels/online-accounts/icons/22x22/Makefile
panels/online-accounts/icons/24x24/Makefile
panels/online-accounts/icons/32x32/Makefile
panels/online-accounts/icons/48x48/Makefile
panels/online-accounts/icons/256x256/Makefile
panels/search/Makefile
panels/search/gnome-search-panel.desktop.in
panels/sharing/Makefile
panels/sharing/gnome-sharing-panel.desktop.in
panels/sound/Makefile
panels/sound/gvc/Makefile
panels/sound/data/Makefile
panels/sound/data/gnome-sound-panel.desktop.in
panels/sound/data/symbolic-icons/Makefile
panels/sound/data/symbolic-icons/scalable/Makefile
panels/sound/data/symbolic-icons/scalable/status/Makefile
panels/sound/data/icons/Makefile
panels/sound/data/icons/16x16/Makefile
panels/sound/data/icons/16x16/apps/Makefile
panels/sound/data/icons/16x16/devices/Makefile
panels/sound/data/icons/16x16/status/Makefile
panels/sound/data/icons/22x22/Makefile
panels/sound/data/icons/22x22/apps/Makefile
panels/sound/data/icons/22x22/status/Makefile
panels/sound/data/icons/24x24/Makefile
panels/sound/data/icons/24x24/apps/Makefile
panels/sound/data/icons/24x24/devices/Makefile
panels/sound/data/icons/24x24/status/Makefile
panels/sound/data/icons/32x32/Makefile
panels/sound/data/icons/32x32/apps/Makefile
panels/sound/data/icons/32x32/devices/Makefile
panels/sound/data/icons/32x32/status/Makefile
panels/sound/data/icons/48x48/Makefile
panels/sound/data/icons/48x48/apps/Makefile
panels/sound/data/icons/48x48/devices/Makefile
panels/sound/data/icons/scalable/Makefile
panels/sound/data/icons/scalable/apps/Makefile
panels/sound/data/icons/scalable/devices/Makefile
panels/sound/data/sounds/Makefile
panels/info/Makefile
panels/info/gnome-info-panel.desktop.in
panels/power/Makefile
panels/power/gnome-power-panel.desktop.in
panels/power/icons/Makefile
panels/power/icons/16x16/Makefile
panels/power/icons/22x22/Makefile
panels/power/icons/24x24/Makefile
panels/power/icons/32x32/Makefile
panels/power/icons/48x48/Makefile
panels/power/icons/256x256/Makefile
panels/color/Makefile
panels/color/gnome-color-panel.desktop.in
panels/color/icons/Makefile
panels/color/icons/16x16/Makefile
panels/color/icons/22x22/Makefile
panels/color/icons/24x24/Makefile
panels/color/icons/32x32/Makefile
panels/color/icons/48x48/Makefile
panels/color/icons/64x64/Makefile
panels/color/icons/256x256/Makefile
panels/color/icons/scalable/Makefile
panels/connman/Makefile
panels/connman/gnome-network-panel.desktop.in
panels/connman/icons/Makefile
panels/connman/icons/16x16/Makefile
panels/connman/icons/22x22/Makefile
panels/connman/icons/24x24/Makefile
panels/connman/icons/32x32/Makefile
panels/connman/icons/48x48/Makefile
panels/connman/icons/64x64/Makefile
panels/connman/icons/scalable/Makefile
panels/printers/Makefile
panels/printers/gnome-printers-panel.desktop.in
panels/privacy/Makefile
panels/privacy/gnome-privacy-panel.desktop.in
panels/notifications/Makefile
panels/notifications/gnome-notifications-panel.desktop.in
panels/universal-access/Makefile
panels/universal-access/gnome-universal-access-panel.desktop.in
panels/user-accounts/Makefile
panels/user-accounts/data/Makefile
panels/user-accounts/data/gnome-user-accounts-panel.desktop.in
panels/user-accounts/data/faces/Makefile
panels/wacom/Makefile
panels/wacom/calibrator/Makefile
panels/wacom/gnome-wacom-panel.desktop.in
po/Makefile.in
libgd/Makefile
search-provider/Makefile
shell/Makefile
shell/gnome-control-center.desktop.in
man/Makefile
])

AC_OUTPUT

AC_MSG_NOTICE([gnome-control-center was configured with the following options:])
if test "x$have_connman" = "xyes"; then
	AC_MSG_NOTICE([** ConnMan (Network panel)])
else
	AC_MSG_NOTICE([   Network panel disabled])
fi
if test "x$have_libmm_glib" = "xyes"; then
	AC_MSG_NOTICE([** ModemManager1 support (Network panel)])
else
	AC_MSG_NOTICE([   ModemManager1 support disabled])
fi
if test "x$have_bluetooth" = "xyes"; then
	AC_MSG_NOTICE([** gnome-bluetooth (Bluetooth panel)])
else
	AC_MSG_NOTICE([   Bluetooth panel disabled])
fi
if test "x$enable_cups" = "xyes"; then
	AC_MSG_NOTICE([** CUPS (Printers panel)])
else
	AC_MSG_NOTICE([   Printers panel disabled])
fi
if test "x$have_cheese" = "xyes"; then
	AC_MSG_NOTICE([** Cheese (Users panel webcam support)])
else
	AC_MSG_NOTICE([   Users panel webcam support disabled])
fi
if test "x$with_libsocialweb" = "xyes"; then
	AC_MSG_NOTICE([** libsocialweb (Background panel Flickr support)])
else
	AC_MSG_NOTICE([   Background panel Flickr support disabled])
fi
if test "x$have_wacom" = "xyes"; then
	AC_MSG_NOTICE([** wacom (Wacom tablet panel)])
else
	AC_MSG_NOTICE([   Wacom panel disabled])
fi
if test "x$enable_ibus" == "xyes"; then
	AC_MSG_NOTICE([** IBus (Region panel IBus support)])
else
	AC_MSG_NOTICE([   Region panel IBus support disabled])
fi
AC_MSG_NOTICE([End options])
