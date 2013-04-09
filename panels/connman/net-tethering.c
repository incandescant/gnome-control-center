/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Intel, Inc
 *
 * Licensed under the GNU General Public License Version 2
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
 */

#include "config.h"
#include <stdlib.h>

#include <glib-object.h>
#include <glib/gi18n.h>

#include "net-tethering.h"
#include "cc-network-resources.h"
#include "cc-network-panel.h"

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

enum {
        DONE,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NetTethering, net_tethering, G_TYPE_OBJECT)

static void
net_tethering_update_apply (NetTethering *tethering)
{
}

static void
cancel_tethering (NetTethering *tethering)
{
        g_signal_emit (tethering, signals[DONE], 0, FALSE);
}

static void
apply_tethering (NetTethering *tethering)
{
}

static void
net_tethering_init (NetTethering *tethering)
{
        GError *error = NULL;
        GtkButton *button;

        tethering->builder = gtk_builder_new ();

        gtk_builder_add_from_resource (tethering->builder,
                                       "/org/gnome/control-center/network/tethering.ui",
                                       &error);
        if (error != NULL) {
                g_warning ("Could not load ui file: %s", error->message);
                g_error_free (error);
                return;
        }

        tethering->window = GTK_WIDGET (gtk_builder_get_object (tethering->builder, "dialog_tethering"));

        gtk_image_set_from_file (GTK_IMAGE (WID (tethering->builder, "image_tethering")), TETHERDIR "/tethering_inactive.png");

        button = GTK_WIDGET (gtk_builder_get_object (tethering->builder, "button_cancel"));
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (cancel_tethering), tethering);

        button = GTK_WIDGET (gtk_builder_get_object (tethering->builder, "button_apply"));
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (apply_tethering), tethering);
}

static void
net_tethering_finalize (GObject *object)
{
        NetTethering *tethering = NET_TETHERING (object);

        if (tethering->window) {
                gtk_widget_destroy (tethering->window);
                tethering->window = NULL;
        }
        g_clear_object (&tethering->parent_window);
        g_clear_object (&tethering->builder);
        g_clear_object (&tethering->panel);

        G_OBJECT_CLASS (net_tethering_parent_class)->finalize (object);
}

static void
net_tethering_class_init (NetTetheringClass *class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (class);

        g_resources_register (cc_network_get_resource ());

        object_class->finalize = net_tethering_finalize;

        signals[DONE] = g_signal_new ("done",
                                      G_OBJECT_CLASS_TYPE (object_class),
                                      G_SIGNAL_RUN_FIRST,
                                      G_STRUCT_OFFSET (NetTetheringClass, done),
                                      NULL, NULL,
                                      NULL,
                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

void
net_tethering_setup (CcNetworkPanel *panel)
{
        CcNetworkPanelPrivate *priv = panel->priv;

}

NetTethering *
net_tethering_new (CcNetworkPanel *panel)
{
        NetTethering *tethering;
        GtkWidget *window;

        tethering = g_object_new (NET_TYPE_TETHERING, NULL);

        if (panel) {
                tethering->panel = g_object_ref (panel);
                window = cc_shell_get_toplevel (cc_panel_get_shell (CC_PANEL (panel)));
                tethering->parent_window = g_object_ref (window);

                gtk_window_set_transient_for (GTK_WINDOW (tethering->window),
                                              GTK_WINDOW (tethering->parent_window));
        }

        return tethering;
}
