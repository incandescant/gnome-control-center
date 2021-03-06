/*
 * Copyright (C) 2013 Intel, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Author: Thomas Wood <thomas.wood@intel.com>
 *
 */

#include "cc-media-sharing.h"

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <glib/gstdio.h>

#define BUS_NAME "org.gnome.Rygel1"
#define OBJECT_PATH "/org/gnome/Rygel1"
#define INTERFACE_NAME "org.gnome.Rygel1"
#define RYGEL_DESKTOP_ID "rygel.desktop"

static GKeyFile*
cc_media_sharing_open_key_file (void)
{
  gchar *path;
  GKeyFile *file;

  file = g_key_file_new ();

  path = g_build_filename (g_get_user_config_dir (), "rygel.conf", NULL);

  g_key_file_load_from_file (file, path,
                             G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                             NULL);

  g_free (path);

  return file;
}

static void
cc_media_sharing_enable_autostart (void)
{
  const gchar *source;
  gchar *destination;
  GDesktopAppInfo *info;
  GFile *file;

  info = g_desktop_app_info_new (RYGEL_DESKTOP_ID);
  if (!info)
    return;

  /* start rygel */
  g_app_info_launch (G_APP_INFO (info), NULL, NULL, NULL);

  /* create a symbolic link to the rygel desktop file in the autostart
   * directory */
  source = g_desktop_app_info_get_filename (info);

  destination = g_build_filename (g_get_user_config_dir (), "autostart",
                                  RYGEL_DESKTOP_ID, NULL);
  file = g_file_new_for_path (destination);
  g_free (destination);

  g_file_make_symbolic_link (file, source, NULL, NULL);

  g_object_unref (info);
  g_object_unref (file);
}

static void
cc_media_sharing_bus_ready_callback (GObject      *object,
                                     GAsyncResult *result,
                                     gpointer      user_data)
{
  GDBusConnection *connection;

  connection = g_bus_get_finish (result, NULL);

  if (!connection)
    return;

  g_dbus_connection_call (connection, BUS_NAME, OBJECT_PATH, INTERFACE_NAME,
                          "Shutdown", NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1,
                          NULL, NULL, NULL);
}

static void
cc_media_sharing_disable_autostart (void)
{
  gchar *path;

  path = g_build_filename (g_get_user_config_dir (), "autostart",
                           RYGEL_DESKTOP_ID, NULL);

  g_unlink (path);
  g_free (path);

  /* stop rygel */
  g_bus_get (G_BUS_TYPE_SESSION, NULL, cc_media_sharing_bus_ready_callback,
             NULL);
}

void
cc_media_sharing_get_preferences (gboolean   *enabled,
                                  gchar    ***folders)
{
  GKeyFile *file;

  file = cc_media_sharing_open_key_file ();

  if (enabled)
   *enabled = g_key_file_get_boolean (file, "general", "upnp-enabled", NULL);

  if (folders)
    {
      gsize length;
      gchar **str_list;

      str_list = g_key_file_get_string_list (file, "MediaExport", "uris",
                                             &length, NULL);

      *folders = str_list;

      while (str_list && *str_list)
        {
          if (g_str_equal (*str_list, "@MUSIC@"))
            {
              g_free (*str_list);
              *str_list = g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_MUSIC));
            }

          if (g_str_equal (*str_list, "@VIDEOS@"))
            {
              g_free (*str_list);
              *str_list = g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS));
            }

          if (g_str_equal (*str_list, "@PICTURES@"))
            {
              g_free (*str_list);
              *str_list = g_strdup (g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));
            }
          str_list++;
        }
    }

  g_key_file_free (file);
}

void
cc_media_sharing_set_preferences (gboolean   enabled,
                                  gchar    **folders)
{
  GKeyFile *file;
  gchar **str_list;
  gchar *path;
  gsize length;
  gchar *data;

  file = cc_media_sharing_open_key_file ();

  if (enabled)
    cc_media_sharing_enable_autostart ();
  else
    cc_media_sharing_disable_autostart ();

  g_key_file_set_boolean (file, "general", "upnp-enabled", enabled);

  str_list = folders;
  length = 0;

  while (str_list && *str_list)
    {
      if (g_str_equal (*str_list, g_get_user_special_dir (G_USER_DIRECTORY_MUSIC)))
        {
          g_free (*str_list);
          *str_list = g_strdup ("@MUSIC@");
        }

      if (g_str_equal (*str_list, g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS)))
        {
          g_free (*str_list);
          *str_list = g_strdup ("@VIDEOS@");
        }

      if (g_str_equal (*str_list, g_get_user_special_dir (G_USER_DIRECTORY_PICTURES)))
        {
          g_free (*str_list);
          *str_list = g_strdup ("@PICTURES@");
        }

      str_list++;
      length++;
    }

  g_key_file_set_string_list (file, "MediaExport", "uris", (const gchar**) folders, length);

  data = g_key_file_to_data (file, NULL, NULL);

  path = g_build_filename (g_get_user_config_dir (), "rygel.conf", NULL);

  g_file_set_contents (path, data, -1, NULL);

  g_free (path);

  g_key_file_free (file);
}
