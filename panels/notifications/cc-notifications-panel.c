/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2012 Giovanni Campagna <scampa.giovanni@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include <egg-list-box/egg-list-box.h>
#include "cc-notifications-panel.h"
#include "cc-notifications-resources.h"
#include "cc-edit-dialog.h"

#define MASTER_SCHEMA "org.gnome.desktop.notifications"
#define APP_SCHEMA MASTER_SCHEMA ".application"
#define APP_PREFIX "/org/gnome/desktop/notifications/application/"

struct _CcNotificationsPanel {
  CcPanel parent_instance;

  GSettings *master_settings;
  GtkBuilder *builder;
  EggListBox *list_box;

  GCancellable *apps_load_cancellable;

  GHashTable *known_applications;
};

struct _CcNotificationsPanelClass {
  CcPanelClass parent;
};

typedef struct {
  char *canonical_app_id;
  GAppInfo *app_info;
  GSettings *settings;

  /* Temporary pointer, to pass from the loading thread
     to the app */
  CcNotificationsPanel *panel;
} Application;

static void application_free (Application *app);
static void build_app_store (CcNotificationsPanel *panel);
static void select_app      (EggListBox *box, GtkWidget *child, CcNotificationsPanel *panel);
static int  sort_apps       (gconstpointer one, gconstpointer two, gpointer user_data);

CC_PANEL_REGISTER (CcNotificationsPanel, cc_notifications_panel);

static void
cc_notifications_panel_dispose (GObject *object)
{
  CcNotificationsPanel *panel = CC_NOTIFICATIONS_PANEL (object);

  g_clear_object (&panel->builder);
  g_clear_object (&panel->master_settings);
  g_clear_pointer (&panel->known_applications, g_hash_table_unref);

  g_cancellable_cancel (panel->apps_load_cancellable);

  G_OBJECT_CLASS (cc_notifications_panel_parent_class)->dispose (object);
}

static void
cc_notifications_panel_finalize (GObject *object)
{
  CcNotificationsPanel *panel = CC_NOTIFICATIONS_PANEL (object);

  g_clear_object (&panel->apps_load_cancellable);

  G_OBJECT_CLASS (cc_notifications_panel_parent_class)->finalize (object);
}

static void
update_separator_func (GtkWidget **separator,
                       GtkWidget  *child,
                       GtkWidget  *before,
                       gpointer    user_data)
{
  if (*separator == NULL && before != NULL)
    {
      *separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);

      /* https://bugzilla.gnome.org/show_bug.cgi?id=690545 */
      g_object_ref_sink (*separator);
      gtk_widget_show (*separator);
    }
}

static void
cc_notifications_panel_init (CcNotificationsPanel *panel)
{
  GtkWidget *w;
  GError *error = NULL;

  g_resources_register (cc_notifications_get_resource ());
  panel->known_applications = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     NULL, g_free);

  panel->builder = gtk_builder_new ();
  if (gtk_builder_add_from_resource (panel->builder,
                                     "/org/gnome/control-center/notifications/notifications.ui",
                                     &error) == 0)
    {
      g_error ("Error loading UI file: %s", error->message);
      g_error_free (error);
      return;
    }

  panel->master_settings = g_settings_new (MASTER_SCHEMA);

  g_settings_bind (panel->master_settings, "show-banners",
                   gtk_builder_get_object (panel->builder, "ccnotify-switch-banners"),
                   "active", G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (panel->master_settings, "show-in-lock-screen",
                   gtk_builder_get_object (panel->builder, "ccnotify-switch-lock-screen"),
                   "active", G_SETTINGS_BIND_DEFAULT);

  panel->list_box = egg_list_box_new ();
  w = GTK_WIDGET (gtk_builder_get_object (panel->builder,
                                          "ccnotify-app-scrolledwindow"));

  egg_list_box_add_to_scrolled (panel->list_box, GTK_SCROLLED_WINDOW (w));
  egg_list_box_set_selection_mode (panel->list_box, GTK_SELECTION_NONE);
  egg_list_box_set_sort_func (panel->list_box, sort_apps, NULL, NULL);
  egg_list_box_set_separator_funcs (panel->list_box,
                                    update_separator_func,
                                    NULL, NULL);

  g_signal_connect (panel->list_box, "child-activated",
                    G_CALLBACK (select_app), panel);

  gtk_widget_set_visible (GTK_WIDGET (panel->list_box), TRUE);

  build_app_store (panel);

  w = GTK_WIDGET (gtk_builder_get_object (panel->builder,
                                          "ccnotify-main-grid"));
  gtk_widget_reparent (w, GTK_WIDGET (panel));
  gtk_widget_show (w);
}

static const char *
cc_notifications_panel_get_help_uri (CcPanel *panel)
{
  /* TODO */
  return NULL;
}

static void
cc_notifications_panel_class_init (CcNotificationsPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

  panel_class->get_help_uri = cc_notifications_panel_get_help_uri;

  /* Separate dispose() and finalize() functions are necessary
   * to make sure we cancel the running thread before the panel
   * gets finalized */
  object_class->dispose = cc_notifications_panel_dispose;
  object_class->finalize = cc_notifications_panel_finalize;
}

static inline GQuark
application_quark (void)
{
  static GQuark quark;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("cc-application");

  return quark;
}

static gboolean
on_off_label_mapping_get (GValue   *value,
                          GVariant *variant,
                          gpointer  user_data)
{
  g_value_set_string (value, g_variant_get_boolean (variant) ? _("On") : _("Off"));

  return TRUE;
}

static void
add_application (CcNotificationsPanel *panel,
                 Application          *app)
{
  GtkWidget *box, *w;
  GIcon *icon;

  icon = g_app_info_get_icon (app->app_info);
  if (icon == NULL)
    icon = g_themed_icon_new ("application-x-executable");
  else
    g_object_ref (icon);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  g_object_set_qdata_full (G_OBJECT (box), application_quark (),
                           app, (GDestroyNotify) application_free);

  gtk_container_add (GTK_CONTAINER (panel->list_box), box);

  w = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_margin_left (w, 12);
  gtk_container_add (GTK_CONTAINER (box), w);
  g_object_unref (icon);

  w = gtk_label_new (g_app_info_get_name (app->app_info));
  gtk_container_add (GTK_CONTAINER (box), w);

  w = gtk_label_new ("");
  g_settings_bind_with_mapping (app->settings, "enable",
                                w, "label",
                                G_SETTINGS_BIND_GET |
                                G_SETTINGS_BIND_NO_SENSITIVITY,
                                on_off_label_mapping_get,
                                NULL,
                                NULL,
                                NULL);
  gtk_widget_set_margin_right (w, 12);
  gtk_widget_set_valign (w, GTK_ALIGN_CENTER);
  gtk_box_pack_end (GTK_BOX (box), w, FALSE, FALSE, 0);

  gtk_widget_show_all (box);

  g_hash_table_add (panel->known_applications, g_strdup (app->canonical_app_id));
}

static void
maybe_add_app_id (CcNotificationsPanel *panel,
                  const char *canonical_app_id)
{
  Application *app;
  gchar *path;
  gchar *full_app_id;
  GSettings *settings;
  GAppInfo *app_info;

  if (g_hash_table_contains (panel->known_applications,
                             canonical_app_id))
    return;

  path = g_strconcat (APP_PREFIX, canonical_app_id, "/", NULL);
  settings = g_settings_new_with_path (APP_SCHEMA, path);

  full_app_id = g_settings_get_string (settings, "application-id");
  app_info = G_APP_INFO (g_desktop_app_info_new (full_app_id));

  app = g_slice_new (Application);
  app->canonical_app_id = g_strdup (canonical_app_id);
  app->settings = settings;
  app->app_info = app_info;

  add_application (panel, app);
  g_free (path);
}

static gboolean
queued_app_info (gpointer data)
{
  Application *app;
  CcNotificationsPanel *panel;

  app = data;
  panel = app->panel;
  app->panel = NULL;

  if (g_cancellable_is_cancelled (panel->apps_load_cancellable) ||
      g_hash_table_contains (panel->known_applications,
                             app->canonical_app_id))
    {
      application_free (app);
      g_object_unref (panel);
      return FALSE;
    }

  g_debug ("Processing queued application %s", app->canonical_app_id);

  add_application (panel, app);
  g_object_unref (panel);

  return FALSE;
}

static char *
app_info_get_id (GAppInfo *app_info)
{
  const char *desktop_id;
  char *ret;
  const char *filename;
  int l;

  desktop_id = g_app_info_get_id (app_info);
  if (desktop_id != NULL)
    {
      ret = g_strdup (desktop_id);
    }
  else
    {
      filename = g_desktop_app_info_get_filename (G_DESKTOP_APP_INFO (app_info));
      ret = g_path_get_basename (filename);
    }

  if (G_UNLIKELY (g_str_has_suffix (ret, ".desktop") == FALSE))
    {
      g_free (ret);
      return NULL;
    }

  l = strlen (desktop_id);
  *(ret + l - strlen(".desktop")) = '\0';
  return ret;
}

static void
process_app_info (CcNotificationsPanel *panel,
                  GTask                *task,
                  GAppInfo             *app_info)
{
  Application *app;
  char *app_id;
  char *canonical_app_id;
  char *path;
  GSettings *settings;
  GSource *source;

  app_id = app_info_get_id (app_info);
  canonical_app_id = g_strcanon (app_id,
                                 "0123456789"
                                 "abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "-",
                                 '-');

  path = g_strconcat (APP_PREFIX, canonical_app_id, "/", NULL);
  settings = g_settings_new_with_path (APP_SCHEMA, path);

  app = g_slice_new (Application);
  app->canonical_app_id = canonical_app_id;
  app->settings = settings;
  app->app_info = g_object_ref (app_info);
  app->panel = g_object_ref (panel);

  source = g_idle_source_new ();
  g_source_set_callback (source, queued_app_info, app, NULL);
  g_source_attach (source, g_task_get_context (task));

  g_free (path);
}

static void
load_apps_thread (GTask        *task,
                  gpointer      panel,
                  gpointer      task_data,
                  GCancellable *cancellable)
{
  GList *iter, *apps;

  apps = g_app_info_get_all ();

  for (iter = apps; iter && !g_cancellable_is_cancelled (cancellable); iter = iter->next)
    {
      GDesktopAppInfo *app;

      app = iter->data;
      if (g_desktop_app_info_get_boolean (app, "X-GNOME-UsesNotifications")) {
        process_app_info (panel, task, G_APP_INFO (app));
        g_debug ("Processing app '%s'", g_app_info_get_id (G_APP_INFO (app)));
      } else {
        g_debug ("Skipped app '%s', doesn't use notifications", g_app_info_get_id (G_APP_INFO (app)));
      }
    }

  g_list_free_full (apps, g_object_unref);
}

static void
load_apps_async (CcNotificationsPanel *panel)
{
  GTask *task;

  panel->apps_load_cancellable = g_cancellable_new ();
  task = g_task_new (panel, panel->apps_load_cancellable, NULL, NULL);
  g_task_run_in_thread (task, load_apps_thread);

  g_object_unref (task);
}

static void
children_changed (GSettings            *settings,
                  const char           *key,
                  CcNotificationsPanel *panel)
{
  int i;
  const gchar **new_app_ids;

  g_settings_get (panel->master_settings,
                  "application-children",
                  "^a&s", &new_app_ids);
  for (i = 0; new_app_ids[i]; i++)
    maybe_add_app_id (panel, new_app_ids[i]);

  g_free (new_app_ids);
}

static void
build_app_store (CcNotificationsPanel *panel)
{
  /* Build application entries for known applications */
  children_changed (panel->master_settings, NULL, panel);
  g_signal_connect (panel->master_settings, "changed::application-children",
                    G_CALLBACK (children_changed), panel);

  /* Scan applications that statically declare to show notifications */
  load_apps_async (panel);
}

static void
select_app (EggListBox           *list_box,
            GtkWidget            *child,
            CcNotificationsPanel *panel)
{
  Application *app;

  app = g_object_get_qdata (G_OBJECT (child), application_quark ());
  cc_build_edit_dialog (panel, app->app_info, app->settings);
}

static void
application_free (Application *app)
{
  g_free (app->canonical_app_id);
  g_object_unref (app->app_info);
  g_object_unref (app->settings);

  g_slice_free (Application, app);
}

static int
sort_apps (gconstpointer one,
           gconstpointer two,
           gpointer      user_data)
{
  Application *a1, *a2;

  a1 = g_object_get_qdata (G_OBJECT (one), application_quark ());
  a2 = g_object_get_qdata (G_OBJECT (two), application_quark ());

  return g_utf8_collate (g_app_info_get_name (a1->app_info),
                         g_app_info_get_name (a2->app_info));
}
