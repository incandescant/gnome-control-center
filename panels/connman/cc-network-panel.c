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

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

enum {
        STATUS_UNAVAILABLE,
        STATUS_OFFLINE,
        STATUS_NOCONNECTIONS,
        STATUS_CONNECTING,
        STATUS_CONNETED,
        STATUS_ONLINE,
        STATUS_TETHERED,
        STATUS_LAST
};

CC_PANEL_REGISTER (CcNetworkPanel, cc_network_panel)

#define NETWORK_PANEL_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_NETWORK_PANEL, CcNetworkPanelPrivate))

struct _CcNetworkPanelPrivate
{
        gint            watch_id;

        GtkBuilder      *builder;
        GCancellable    *cancellable;
};

static void
cc_network_panel_dispose (GObject *object)
{
        CcNetworkPanelPrivate *priv = CC_NETWORK_PANEL (object)->priv;

        if (priv->cancellable != NULL)
                g_cancellable_cancel (priv->cancellable);

        g_clear_object (&priv->cancellable);

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
        default:
                return;
        }
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

        priv->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                           "net.connman",
                                           G_BUS_NAME_WATCHER_FLAGS_NONE,
                                           connman_appeared_cb,
                                           connman_disappeared_cb,
                                           panel,
                                           NULL);
}
