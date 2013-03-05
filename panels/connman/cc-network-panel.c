/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2012 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2012 Thomas Bechtold <thomasbechtold@jpberlin.de>
 * Copyright (C) 2013 Aleksander Morgado <aleksander@gnu.org>
 * Copyright (C) 2013 Alok Barsode <alok.barsode@intel.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "cc-network-panel.h"
#include "cc-network-resources.h"
#include "manager.h"

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

enum {
        STATUS_UNAVAILABLE,
        STATUS_OFFLINE,
        STATUS_IDLE,
        STATUS_CONNECTING,
        STATUS_READY,
        STATUS_ONLINE,
        STATUS_TETHERED,
        STATUS_LAST
};

static gint
status_to_int (const gchar *status)
{
        if (g_strcmp0 (status , "offline") == 0)
                return STATUS_OFFLINE;
        else if (g_strcmp0 (status , "idle") == 0)
                return STATUS_IDLE;
        else if (g_strcmp0 (status , "ready") == 0)
                return STATUS_READY;
        else if (g_strcmp0 (status , "online") == 0)
                return STATUS_ONLINE;
        else
                return STATUS_UNAVAILABLE;
}

static void
on_manager_property_changed (Manager *manager, const gchar *property,
                             GVariant *value, CcNetworkPanel *panel);

CC_PANEL_REGISTER (CcNetworkPanel, cc_network_panel)

#define NETWORK_PANEL_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_NETWORK_PANEL, CcNetworkPanelPrivate))

struct _CcNetworkPanelPrivate
{
        gint            watch_id;

        GtkBuilder      *builder;
        GCancellable    *cancellable;

        Manager         *manager;
};

static void
cc_network_panel_dispose (GObject *object)
{
        CcNetworkPanelPrivate *priv = CC_NETWORK_PANEL (object)->priv;

        if (priv->cancellable != NULL)
                g_cancellable_cancel (priv->cancellable);

        g_clear_object (&priv->cancellable);

        if (priv->watch_id)
                g_bus_unwatch_name (priv->watch_id);
        priv->watch_id = 0;

        if (priv->manager) {
                g_signal_handlers_disconnect_by_func (priv->manager,
                                                      on_manager_property_changed,
                                                      object);
                g_object_unref (priv->manager);
                priv->manager = NULL;
        }

        g_clear_object (&priv->manager);

        G_OBJECT_CLASS (cc_network_panel_parent_class)->dispose (object);
}

static void
cc_network_panel_finalize (GObject *object)
{
        CcNetworkPanel *panel = CC_NETWORK_PANEL (object);

        G_OBJECT_CLASS (cc_network_panel_parent_class)->finalize (object);
}

static const char *
cc_network_panel_get_help_uri (CcPanel *panel)
{
	return "help:gnome-help/net";
}

static void
cc_network_panel_class_init (CcNetworkPanelClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
	CcPanelClass *panel_class = CC_PANEL_CLASS (klass);

        g_type_class_add_private (klass, sizeof (CcNetworkPanelPrivate));

	panel_class->get_help_uri = cc_network_panel_get_help_uri;

        object_class->dispose = cc_network_panel_dispose;
        object_class->finalize = cc_network_panel_finalize;
}

static void
manager_set_offlinemode(GObject      *source,
                        GAsyncResult *res,
                        gpointer      user_data)
{
        CcNetworkPanel *panel = user_data;
        CcNetworkPanelPrivate *priv = panel->priv;
        gboolean offline;
        GError *error = NULL;
        gint err_code;

        if (!manager_call_set_property_finish (priv->manager, res, &error)) {
                g_warning ("Manager: Could not set OfflineMode: %s", error->message);
                err_code = error->code;
                g_error_free (error);

                /* Reset the switch */
                if (err_code != 36) { /* if not AlreadyEnabled */
                        offline = gtk_switch_get_active (GTK_SWITCH (WID (priv->builder, "switch_offline")));
                        gtk_switch_set_active (GTK_SWITCH (WID (priv->builder, "switch_offline")), !offline);
                }

                return;
        }
}

static void
offline_switch_toggle (GtkSwitch *sw,
                       GParamSpec *pspec,
                       CcNetworkPanel *panel)
{
        CcNetworkPanelPrivate *priv = panel->priv;
        gboolean offline;
        GVariant *value;

        offline = gtk_switch_get_active (sw);
        value = g_variant_new_boolean (offline);

        manager_call_set_property (priv->manager,
                                   "OfflineMode",
                                   g_variant_new_variant (value),
                                   priv->cancellable,
                                   manager_set_offlinemode,
                                   panel);
}

static void
network_set_status (CcNetworkPanel *panel, gint status)
{
        CcNetworkPanelPrivate *priv;
        priv = NETWORK_PANEL_PRIVATE (panel);

        switch (status) {
        case STATUS_UNAVAILABLE:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_offline",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("Unavailable"));
                break;
        case STATUS_OFFLINE:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_noconn",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("In-flight mode"));
                break;
        case STATUS_IDLE:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_noconn",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("Idle"));
                break;
        case STATUS_CONNECTING:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_conn",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("Connecting..."));
                break;
        case STATUS_READY:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_online",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("Connected"));
                break;
        case STATUS_ONLINE:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_online",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("Online"));
                break;
        case STATUS_TETHERED:
                gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "icon_status")),
                                      "connman_hotspot",
                                      GTK_ICON_SIZE_BUTTON);
                gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_status")), _("Hotspot"));
                break;
        default:
                return;
        }
}

static void
on_manager_property_changed (Manager *manager,
                             const gchar *property,
                             GVariant *value,
                             CcNetworkPanel *panel)
{
        GVariant *var;
        gboolean offlinemode;
        const gchar *state;

        CcNetworkPanelPrivate *priv;

        priv = NETWORK_PANEL_PRIVATE (panel);
        var = g_variant_get_variant (value);

        if (!g_strcmp0 (property, "OfflineMode")) {
                offlinemode = g_variant_get_boolean (var);
                gtk_switch_set_active (GTK_SWITCH (WID (priv->builder, "switch_offline")), offlinemode);
        }

        if (!g_strcmp0 (property, "State")) {
                state = g_variant_get_string (var, NULL);
                network_set_status (panel, status_to_int (state));
        }
}

static void
manager_get_properties(GObject      *source,
                       GAsyncResult *res,
                       gpointer      user_data)
{
        CcNetworkPanel *panel = user_data;
        CcNetworkPanelPrivate *priv = panel->priv;

        GError *error;
        GVariant *result = NULL;
        GVariant *value = NULL;
        gboolean offlinemode;
        const gchar *state;

        error = NULL;
        if (!manager_call_get_properties_finish (priv->manager, &result,
                                                 res, &error))
                {
                        /* TODO: display any error in a user friendly way */
                        g_warning ("Could not get manager properties: %s", error->message);
                        g_error_free (error);
                        return;
                }

        value = g_variant_lookup_value (result, "OfflineMode", G_VARIANT_TYPE_BOOLEAN);
        offlinemode = g_variant_get_boolean (value);

        gtk_switch_set_active (GTK_SWITCH (WID (priv->builder, "switch_offline")), offlinemode);

        value = g_variant_lookup_value (result, "State", G_VARIANT_TYPE_STRING);
        state = g_variant_get_string (value, NULL);
        network_set_status (panel, status_to_int (state));
}

static void
manager_created_cb (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
        CcNetworkPanel *panel = user_data;
        CcNetworkPanelPrivate *priv = NETWORK_PANEL_PRIVATE (panel);

        GError *error = NULL;

        g_clear_object (&priv->manager);

        priv->manager = manager_proxy_new_for_bus_finish (res, &error);
        if (error != NULL) {
                g_warning ("Couldn't contact connmand service: %s", error->message);
                g_error_free (error);
                return;
        }

        g_signal_connect (priv->manager, "property_changed",
                          G_CALLBACK (on_manager_property_changed), panel);

        manager_call_get_properties (priv->manager, priv->cancellable, manager_get_properties, panel);
}

static void
connman_appeared_cb (GDBusConnection *connection,
                     const gchar *name,
                     const gchar *name_owner,
                     gpointer user_data)
{
        GtkWidget *widget;
        CcNetworkPanel *panel = user_data;
        CcNetworkPanelPrivate *priv;

        if (strcmp (name, "net.connman"))
                return;

        priv = NETWORK_PANEL_PRIVATE (panel);

        widget = GTK_WIDGET (WID (priv->builder, "vbox1"));
        gtk_widget_set_sensitive (widget, TRUE);

        manager_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                                   G_DBUS_PROXY_FLAGS_NONE,
                                   "net.connman",
                                   "/",
                                   priv->cancellable,
                                   manager_created_cb,
                                   panel);
}

static void
connman_disappeared_cb (GDBusConnection *connection,
                        const gchar *name,
                        gpointer user_data)
{
        GtkWidget *widget;
        CcNetworkPanel *panel = user_data;
        CcNetworkPanelPrivate *priv;

        if (strcmp (name, "net.connman"))
                return;

        priv = NETWORK_PANEL_PRIVATE (panel);

        network_set_status (panel, STATUS_UNAVAILABLE);

        widget = GTK_WIDGET (WID (priv->builder, "vbox1"));
        gtk_widget_set_sensitive (widget, FALSE);

        if (priv->manager) {
                g_signal_handlers_disconnect_by_func (priv->manager,
                                                      on_manager_property_changed,
                                                      panel);
                g_object_unref (priv->manager);
                priv->manager = NULL;
        }

}

static void
cc_network_panel_init (CcNetworkPanel *panel)
{
        GError *error = NULL;
        GtkWidget *widget;
        CcNetworkPanelPrivate *priv;

        priv = panel->priv = NETWORK_PANEL_PRIVATE (panel);
        g_resources_register (cc_network_get_resource ());

        panel->priv->builder = gtk_builder_new ();
        gtk_builder_add_from_resource (priv->builder,
                                       "/org/gnome/control-center/network/network.ui",
                                       &error);
        if (error != NULL) {
                g_warning ("Could not load interface file: %s", error->message);
                g_error_free (error);
                return;
        }

        priv->cancellable = g_cancellable_new ();

        widget = GTK_WIDGET (WID (priv->builder, "vbox1"));
        gtk_widget_reparent (widget, (GtkWidget *) panel);

        gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_offline")), _("In-flight Mode"));

        gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_ethernet")), _("Ethernet"));
        gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_wifi")), _("Wireless"));
        gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_bluetooth")), _("Bluetooth"));
        gtk_label_set_text (GTK_LABEL (WID (priv->builder, "label_cellular")), _("Cellular"));

        gtk_button_set_label (GTK_BUTTON (WID (priv->builder, "button_vpn")), _("Add a VPN"));
        gtk_button_set_label (GTK_BUTTON (WID (priv->builder, "button_hotspot")), _("Create HotSpot"));

        gtk_image_set_from_icon_name (GTK_IMAGE (WID (priv->builder, "image_hotspot")),
                                      "connman_hotspot", GTK_ICON_SIZE_BUTTON);

        g_signal_connect (GTK_SWITCH (WID (priv->builder, "switch_offline")),
                          "notify::active",
                          G_CALLBACK (offline_switch_toggle),
                          panel);

        priv->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                           "net.connman",
                                           G_BUS_NAME_WATCHER_FLAGS_NONE,
                                           connman_appeared_cb,
                                           connman_disappeared_cb,
                                           panel,
                                           NULL);
}
