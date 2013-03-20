/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Red Hat, Inc
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

#include <glib-object.h>
#include <glib/gi18n.h>

#include "../service.h"
#include "net-connection-editor.h"
#include "net-connection-editor-resources.h"

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)

enum {
        DONE,
        LAST_SIGNAL
};

enum {
        COLUMN_ICON,
        COLUMN_PULSE,
        COLUMN_PULSE_ID,
        COLUMN_NAME,
        COLUMN_STATE,
        COLUMN_SECURITY_ICON,
        COLUMN_SECURITY,
        COLUMN_TYPE,
        COLUMN_STRENGTH_ICON,
        COLUMN_STRENGTH,
        COLUMN_FAVORITE,
        COLUMN_GDBUSPROXY,
        COLUMN_PROP_ID,
        COLUMN_AUTOCONNECT,
        COLUMN_ETHERNET,
        COLUMN_IPV4,
        COLUMN_NAMESERVERS,
        COLUMN_EDITOR,
        COLUMN_LAST
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NetConnectionEditor, net_connection_editor, G_TYPE_OBJECT)

void
editor_update_details (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;

        gchar link_speed;

        const gchar *signal_strength;
        const gchar *security, *ipv4_address, *mac, *def_route;
        gboolean autoconnect, favorite, ret;
        GVariant *ethernet, *ipv4, *nameservers;
        gchar **ns;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_FAVORITE, &favorite,
                            COLUMN_SECURITY, &security,
                            COLUMN_STRENGTH, &signal_strength,
                            COLUMN_AUTOCONNECT, &autoconnect,
                            COLUMN_ETHERNET, &ethernet,
                            COLUMN_IPV4, &ipv4,
                            COLUMN_NAMESERVERS, &nameservers,
                            -1);

        if (favorite) {
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "switch_autoconnect")), TRUE);
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "button_forget")), TRUE);

                gtk_switch_set_active (GTK_SWITCH (WID (editor->builder, "switch_autoconnect")), autoconnect);
        } else {
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "switch_autoconnect")), FALSE);
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "button_forget")), FALSE);
        }

        gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_strength")), signal_strength);

        gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_security")), g_ascii_strup (security, -1));

        ret = g_variant_lookup (ethernet, "Speed", "y", &link_speed);
        if (ret)
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_link_speed")), g_strdup_printf ("%c MB/sec", link_speed));
        else
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_link_speed")), "--:--");

        ret = g_variant_lookup (ethernet, "Address", "s", &mac);
        if (ret)
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_mac")), mac);
        else
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_mac")), "N/A");

        ret = g_variant_lookup (ipv4, "Address", "s", &ipv4_address);
        if (ret)
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_ipv4_address")), ipv4_address);
        else
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_ipv4_address")), "N/A");

        ret = g_variant_lookup (ipv4, "Gateway", "s", &def_route);
        if (ret)
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_default_route")), def_route);
        else
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_default_route")), "N/A");

        ns = g_variant_dup_strv (nameservers, NULL);
        if (ns && ns[0])
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_dns")), g_strjoinv (",", (gchar **) ns));
        else
                gtk_label_set_text (GTK_LABEL (WID (editor->builder, "label_dns")), "N/A");
}

static void
service_set_autoconnect (GObject      *source,
                         GAsyncResult *res,
                         gpointer      user_data)
{
        NetConnectionEditor *editor = user_data;
        gboolean ac;
        GError *error = NULL;

        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;
        Service *service;

        if (!editor)
                return;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GDBUSPROXY, &service,
                            -1);

        if (!service_call_set_property_finish (service, res, &error)) {
                gchar *err = g_dbus_error_get_remote_error (error);

                if (!g_strcmp0 (err, "net.connman.Error.AlreadyEnabled") || !g_strcmp0 (err, "net.connman.Error.AlreadyDisabled"))
                        goto done;

                ac = gtk_switch_get_active (GTK_SWITCH (WID (editor->builder, "switch_autoconnect")));

                gtk_switch_set_active (GTK_SWITCH (WID (editor->builder, "switch_autoconnect")), !ac);

                g_warning ("Could not set Service AutoConnect property: %s", error->message);
done:
                g_error_free (error);
                g_free (err);
                return;
        }
}

static void
autoconnect_toggle (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;
        Service *service;
        gboolean autoconnect, ac;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GDBUSPROXY, &service,
                            COLUMN_AUTOCONNECT, &autoconnect,
                            -1);

        ac = gtk_switch_get_active (GTK_SWITCH (WID (editor->builder, "switch_autoconnect")));
        if (ac == autoconnect)
                return;

        service_call_set_property (service,
                                   "AutoConnect",
                                   g_variant_new_variant (g_variant_new_boolean (ac)),
                                   NULL,
                                   service_set_autoconnect,
                                   editor);
}

static void
service_removed (GObject      *source,
                 GAsyncResult *res,
                 gpointer      user_data)
{
        NetConnectionEditor *editor = user_data;
        GError *error = NULL;

        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;
        Service *service;

        if (!editor)
                return;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GDBUSPROXY, &service,
                            -1);

        if (!service_call_remove_finish (service, res, &error)) {
                g_warning ("Could not remove Service: %s", error->message);
                g_error_free (error);
                return;
        }
}

static void
forget_service (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;
        Service *service;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GDBUSPROXY, &service,
                            -1);

        service_call_remove (service,
                             NULL,
                             service_removed,
                             editor);
}

static void
cancel_editing (NetConnectionEditor *editor)
{
        g_signal_emit (editor, signals[DONE], 0, FALSE);
}

static void
apply_edits (NetConnectionEditor *editor)
{
}

static void
selection_changed (GtkTreeSelection *selection, NetConnectionEditor *editor)
{
        //GtkWidget *widget;
        GtkTreeModel *model;
        GtkTreeIter iter;
        gint page;

        gtk_tree_selection_get_selected (selection, &model, &iter);
        gtk_tree_model_get (model, &iter, 1, &page, -1);

        /* g_printerr ("\n %d page selected", page); */
        /* widget = GTK_WIDGET (gtk_builder_get_object (editor->builder, */
        /*                                              "details_notebook")); */
        /* gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), page); */
}

static void
net_connection_editor_init (NetConnectionEditor *editor)
{
        GError *error = NULL;
        GtkTreeSelection *selection;
        GtkWidget *button;

        editor->builder = gtk_builder_new ();

        gtk_builder_add_from_resource (editor->builder,
                                       "/org/gnome/control-center/network/connection-editor.ui",
                                       &error);
        if (error != NULL) {
                g_warning ("Could not load ui file: %s", error->message);
                g_error_free (error);
                return;
        }

        editor->window = GTK_WIDGET (gtk_builder_get_object (editor->builder, "dialog_ce"));
        selection = GTK_TREE_SELECTION (gtk_builder_get_object (editor->builder,
                                                                "treeview-selection"));
        g_signal_connect (selection, "changed",
                          G_CALLBACK (selection_changed), editor);

        button = GTK_WIDGET (gtk_builder_get_object (editor->builder, "cancel_button"));
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (cancel_editing), editor);

        g_signal_connect_swapped (editor->window, "delete-event",
                                  G_CALLBACK (cancel_editing), editor);

        button = GTK_WIDGET (gtk_builder_get_object (editor->builder, "apply_button"));
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (apply_edits), editor);

        button = GTK_WIDGET (gtk_builder_get_object (editor->builder, "switch_autoconnect"));
        g_signal_connect_swapped (button, "notify::active",
                                  G_CALLBACK (autoconnect_toggle), editor);

        button = GTK_WIDGET (gtk_builder_get_object (editor->builder, "button_forget"));
        g_signal_connect_swapped (button, "clicked",
                                  G_CALLBACK (forget_service), editor);
}

static void
net_connection_editor_finalize (GObject *object)
{
        NetConnectionEditor *editor = NET_CONNECTION_EDITOR (object);

        if (editor->window) {
                gtk_widget_destroy (editor->window);
                editor->window = NULL;
        }
        g_clear_object (&editor->parent_window);
        g_clear_object (&editor->builder);

        G_OBJECT_CLASS (net_connection_editor_parent_class)->finalize (object);
}

static void
net_connection_editor_class_init (NetConnectionEditorClass *class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (class);

        g_resources_register (net_connection_editor_get_resource ());

        object_class->finalize = net_connection_editor_finalize;

        signals[DONE] = g_signal_new ("done",
                                      G_OBJECT_CLASS_TYPE (object_class),
                                      G_SIGNAL_RUN_FIRST,
                                      G_STRUCT_OFFSET (NetConnectionEditorClass, done),
                                      NULL, NULL,
                                      NULL,
                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

NetConnectionEditor *
net_connection_editor_new (GtkWindow *parent_window,
                           GtkTreeRowReference *row)
{
        NetConnectionEditor *editor;

        editor = g_object_new (NET_TYPE_CONNECTION_EDITOR, NULL);

        if (parent_window) {
                editor->parent_window = g_object_ref (parent_window);
                gtk_window_set_transient_for (GTK_WINDOW (editor->window),
                                              parent_window);
        }

        editor->service_row = row;

        editor_update_details (editor);

        gtk_window_present (GTK_WINDOW (editor->window));

        return editor;
}
