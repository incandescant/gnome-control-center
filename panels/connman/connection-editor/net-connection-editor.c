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
        COLUMN_PROXY,
        COLUMN_EDITOR,
        COLUMN_LAST
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (NetConnectionEditor, net_connection_editor, G_TYPE_OBJECT)

static void
net_connection_editor_update_apply (NetConnectionEditor *editor)
{

        if (editor->update_proxy || editor->update_ipv4)
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "apply_button")), TRUE);
        else
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "apply_button")), FALSE);
}

/* Details section */
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

        net_connection_editor_update_apply (editor);

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
/* Details section Ends */

/* Proxy section */

static void
proxy_setup_entry (NetConnectionEditor *editor, gchar *method)
{
        editor->proxy_method = method;

        if (!g_strcmp0 (method, "direct")) {
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "proxy_url")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "proxy_servers")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "proxy_excludes")));

                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_url")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_servers")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_excludes")));

        } else if (!g_strcmp0 (method, "auto")) {
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "label_url")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "proxy_url")));

                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_servers")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "proxy_servers")));

                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_excludes")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "proxy_excludes")));
        } else if (!g_strcmp0 (method, "manual")) {
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_url")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "proxy_url")));

                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "label_servers")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "proxy_servers")));

                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "label_excludes")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "proxy_excludes")));
        } else
                g_warning ("Unknown method for proxy"); 
}

static void
proxy_method_changed (GtkComboBox *combo, gpointer user_data)
{
        NetConnectionEditor *editor = user_data;
        gint active;

        active = gtk_combo_box_get_active (combo);
        if (active == 0) {
                proxy_setup_entry (editor, "direct");

                editor->update_proxy = TRUE;
                net_connection_editor_update_apply (editor);
        } else if (active == 1) {
                proxy_setup_entry (editor, "auto");

                editor->update_proxy = TRUE;
                net_connection_editor_update_apply (editor);
        } else
                proxy_setup_entry (editor, "manual");
}

static void
proxy_servers_text_changed (NetConnectionEditor *editor)
{
        GtkEntry *entry;
        guint16 len;

        if (g_strcmp0 (editor->proxy_method, "manual") != 0)
                return;

        entry = GTK_ENTRY (WID (editor->builder, "proxy_servers"));

        len = gtk_entry_get_text_length (entry);
        if (len == 0)
                editor->update_proxy = FALSE;
        else
                editor->update_proxy = TRUE;

        net_connection_editor_update_apply (editor);
}

void
editor_update_proxy (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;

        gboolean ret;
        gchar *method, *url;
        gchar **servers, **excludes;
        GVariant *proxy, *value;
        guint16 len;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_PROXY, &proxy,
                            -1);

        ret = g_variant_lookup (proxy, "Method", "s", &method);
        if (!ret)
                method = "direct";

        proxy_setup_entry (editor, method);

        if (!g_strcmp0 (method, "direct"))
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_proxy_method")), 0);
        else  if (!g_strcmp0 (method, "auto")) {
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_proxy_method")), 1);
                g_variant_lookup (proxy, "URL", "s", &url);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "proxy_url")), url);
        } else {
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_proxy_method")), 2);

                value = g_variant_lookup_value (proxy, "Servers", G_VARIANT_TYPE_STRING_ARRAY);
                servers = g_variant_get_strv (value, NULL);

                value = g_variant_lookup_value (proxy, "Excludes", G_VARIANT_TYPE_STRING_ARRAY);
                excludes = g_variant_get_strv (value, NULL);

                if (servers != NULL)
                        gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "proxy_servers")), g_strjoinv (",", (gchar **) servers));

                if (excludes != NULL)
                        gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "proxy_excludes")), g_strjoinv (",", (gchar **) excludes));
        }

        editor->update_proxy = FALSE;
        net_connection_editor_update_apply (editor);
}

static void
service_set_proxy (GObject      *source,
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

        if (!service_call_set_property_finish (service, res, &error)) {
                g_warning ("Could not set proxy: %s", error->message);
                g_error_free (error);
                return;
        }
}

static void
editor_set_proxy (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;
        Service *service;
        gint active;
        GVariantBuilder *proxyconf = g_variant_builder_new (G_VARIANT_TYPE_DICTIONARY);
        gchar *str;
        gchar **servers = NULL;
        gchar **excludes = NULL;
        GVariant *value;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GDBUSPROXY, &service,
                            -1);


        active = gtk_combo_box_get_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_proxy_method")));
        if (active == 0) {
                g_variant_builder_add (proxyconf,"{sv}", "Method", g_variant_new_string ("direct"));
        } else if (active == 1) {
                g_variant_builder_add (proxyconf,"{sv}", "Method", g_variant_new_string ("auto"));
                str = (gchar *) gtk_entry_get_text (GTK_ENTRY (WID (editor->builder, "proxy_url")));
                if (str)
                        g_variant_builder_add (proxyconf,"{sv}", "URL", g_variant_new_string (str));
        } else {
                g_variant_builder_add (proxyconf,"{sv}", "Method", g_variant_new_string ("manual"));

                str = (gchar *) gtk_entry_get_text (GTK_ENTRY (WID (editor->builder, "proxy_servers")));
                if (str) {
                        servers = g_strsplit (str, ",", -1);
                        g_variant_builder_add (proxyconf,"{sv}", "Servers", g_variant_new_strv ((const gchar * const *) servers, -1));
                }

                str = (gchar *) gtk_entry_get_text (GTK_ENTRY (WID (editor->builder, "proxy_excludes")));
                if (str) {
                        excludes = g_strsplit (str, ",", -1);
                        g_variant_builder_add (proxyconf,"{sv}", "Excludes", g_variant_new_strv ((const gchar * const *) excludes, -1));
                }
        }

        value = g_variant_builder_end (proxyconf);

        service_call_set_property (service,
                                  "Proxy.Configuration",
                                   g_variant_new_variant (value),
                                   NULL,
                                   service_set_proxy,
                                   editor);

        g_variant_builder_unref (proxyconf);
 
        if (servers)
                g_strfreev (servers);

        if (excludes)
                g_strfreev (excludes);
}

/* Proxy section Ends */

/* IPv4 section */

static void
ipv4_setup_entry (NetConnectionEditor *editor, gchar *method)
{
        editor->ipv4_method = method;

        if (!g_strcmp0 (method, "off")) {
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "ipv4_address")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "ipv4_netmask")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "ipv4_gateway")));

                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_ipv4_addr")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_ipv4_netmask")));
                gtk_widget_hide (GTK_WIDGET (WID (editor->builder, "label_ipv4_gateway")));
                return;
        } else {
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "ipv4_address")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "ipv4_netmask")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "ipv4_gateway")));

                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "label_ipv4_addr")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "label_ipv4_netmask")));
                gtk_widget_show (GTK_WIDGET (WID (editor->builder, "label_ipv4_gateway")));
        }

        if (!g_strcmp0 (method, "dhcp") || !g_strcmp0 (method, "fixed")) {
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "ipv4_address")), FALSE);
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "ipv4_netmask")), FALSE);
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "ipv4_gateway")), FALSE);
                return;
        }

        if (!g_strcmp0 (method, "manual")) {
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "ipv4_address")), TRUE);
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "ipv4_netmask")), TRUE);
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "ipv4_gateway")), TRUE);
        }
}

static void
ipv4_method_changed (GtkComboBox *combo, gpointer user_data)
{
        NetConnectionEditor *editor = user_data;
        gint active;

        active = gtk_combo_box_get_active (combo);
        if (active == 0) {
                ipv4_setup_entry (editor, "off");
                editor->update_ipv4 = TRUE;
                net_connection_editor_update_apply (editor);
        } else if (active == 1) {
                ipv4_setup_entry (editor, "dhcp");

                editor->update_ipv4 = TRUE;
                net_connection_editor_update_apply (editor);
        } else if (active == 2) {
                ipv4_setup_entry (editor, "manual");
        } else {
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")), 0);
        }
}

static void
ipv4_address_text_changed (NetConnectionEditor *editor)
{
        GtkEntry *entry;
        guint16 len1, len2, len3;

        if (g_strcmp0 (editor->ipv4_method, "manual") != 0)
                return;

        entry = GTK_ENTRY (WID (editor->builder, "ipv4_address"));
        len1 = gtk_entry_get_text_length (entry);

        entry = GTK_ENTRY (WID (editor->builder, "ipv4_netmask"));
        len2 = gtk_entry_get_text_length (entry);

        entry = GTK_ENTRY (WID (editor->builder, "ipv4_gateway"));
        len3 = gtk_entry_get_text_length (entry);

        if (len1 && len2 && len3)
                editor->update_ipv4 = TRUE;
        else
                editor->update_ipv4 = FALSE;

        net_connection_editor_update_apply (editor);
}

void
editor_update_ipv4 (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;

        gboolean ret;
        gchar *method, *address, *netmask, *gateway;
        GVariant *ipv4, *value;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_IPV4, &ipv4,
                            -1);

        ret = g_variant_lookup (ipv4, "Method", "s", &method);
        if (!ret)
                method = "off";

        ipv4_setup_entry (editor, method);

        if (!g_strcmp0 (method, "off"))
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")), 0);
        else if (!g_strcmp0 (method, "dhcp")) {
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")), 1);

                g_variant_lookup (ipv4, "Address", "s", &address);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_address")), address);

                g_variant_lookup (ipv4, "Netmask", "s", &netmask);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_netmask")), netmask);

                g_variant_lookup (ipv4, "Gateway", "s", &gateway);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_gateway")), gateway);
        } else if (!g_strcmp0 (method, "manual")) {
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")), 2);

                g_variant_lookup (ipv4, "Address", "s", &address);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_address")), address);

                g_variant_lookup (ipv4, "Netmask", "s", &netmask);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_netmask")), netmask);

                g_variant_lookup (ipv4, "Gateway", "s", &gateway);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_gateway")), gateway);
        } else {
                gtk_combo_box_set_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")), 3);

                g_variant_lookup (ipv4, "Address", "s", &address);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_address")), address);

                g_variant_lookup (ipv4, "Netmask", "s", &netmask);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_netmask")), netmask);

                g_variant_lookup (ipv4, "Gateway", "s", &gateway);
                gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_gateway")), gateway);
        }

        if (!g_strcmp0 (method, "fixed"))
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "comboboxtext_ipv4_method")), FALSE);
        else
                gtk_widget_set_sensitive (GTK_WIDGET (WID (editor->builder, "comboboxtext_ipv4_method")), TRUE);

        editor->update_ipv4 = FALSE;
        net_connection_editor_update_apply (editor);
}

static void
service_set_ipv4 (GObject      *source,
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

        if (!service_call_set_property_finish (service, res, &error)) {
                g_warning ("Could not set ipv4: %s", error->message);
                g_error_free (error);
                return;
        }
}

static void
editor_set_ipv4 (NetConnectionEditor *editor)
{
        GtkTreeModel *model;

        GtkTreePath *tree_path;
        GtkTreeIter iter;
        Service *service;
        gint active;

        GVariantBuilder *ipv4conf = g_variant_builder_new (G_VARIANT_TYPE_DICTIONARY);
        gchar *str;
        GVariant *value;

        model =  gtk_tree_row_reference_get_model (editor->service_row);
        tree_path = gtk_tree_row_reference_get_path (editor->service_row);
        gtk_tree_model_get_iter (model, &iter, tree_path);

        gtk_tree_model_get (model, &iter,
                            COLUMN_GDBUSPROXY, &service,
                            -1);

        active = gtk_combo_box_get_active (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")));

        if (active == 0) {
                g_variant_builder_add (ipv4conf,"{sv}", "Method", g_variant_new_string ("off"));
        } else if (active == 1) {
                g_variant_builder_add (ipv4conf,"{sv}", "Method", g_variant_new_string ("dhcp"));
        } else {
                g_variant_builder_add (ipv4conf,"{sv}", "Method", g_variant_new_string ("manual"));

                str = (gchar *) gtk_entry_get_text (GTK_ENTRY (WID (editor->builder, "ipv4_address")));
                g_variant_builder_add (ipv4conf,"{sv}", "Address", g_variant_new_string (str));

                str = (gchar *) gtk_entry_get_text (GTK_ENTRY (WID (editor->builder, "ipv4_netmask")));
                g_variant_builder_add (ipv4conf,"{sv}", "Netmask", g_variant_new_string (str));

                str = (gchar *) gtk_entry_get_text (GTK_ENTRY (WID (editor->builder, "ipv4_gateway")));
                g_variant_builder_add (ipv4conf,"{sv}", "Gateway", g_variant_new_string (str));
        }

        value = g_variant_builder_end (ipv4conf);

        gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_address")), "");
        gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_netmask")), "");
        gtk_entry_set_text (GTK_ENTRY (WID (editor->builder, "ipv4_gateway")), "");

        service_call_set_property (service,
                                  "IPv4.Configuration",
                                   g_variant_new_variant (value),
                                   NULL,
                                   service_set_ipv4,
                                   editor);

        g_variant_builder_unref (ipv4conf);
}

/* IPv4 section Ends */

static void
cancel_editing (NetConnectionEditor *editor)
{
        g_signal_emit (editor, signals[DONE], 0, FALSE);
}

static void
apply_edits (NetConnectionEditor *editor)
{

        if (editor->update_proxy)
               editor_set_proxy (editor);
        if (editor->update_ipv4)
               editor_set_ipv4 (editor);
}

static void
selection_changed (GtkTreeSelection *selection, NetConnectionEditor *editor)
{
        GtkWidget *widget;
        GtkTreeModel *model;
        GtkTreeIter iter;
        gint page;

        gtk_tree_selection_get_selected (selection, &model, &iter);
        gtk_tree_model_get (model, &iter, 1, &page, -1);

        widget = GTK_WIDGET (gtk_builder_get_object (editor->builder,
                                                     "details_notebook"));
        gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), page);
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

        g_signal_connect (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_proxy_method")),
                          "changed",
                          G_CALLBACK (proxy_method_changed),
                          editor);

        g_signal_connect_swapped (GTK_ENTRY (WID (editor->builder, "proxy_servers")),
                                  "notify::text-length",
                                  G_CALLBACK (proxy_servers_text_changed),
                                  editor);

        g_signal_connect (GTK_COMBO_BOX (WID (editor->builder, "comboboxtext_ipv4_method")),
                          "changed",
                          G_CALLBACK (ipv4_method_changed),
                          editor);

        g_signal_connect_swapped (GTK_ENTRY (WID (editor->builder, "ipv4_address")),
                                  "notify::text-length",
                                  G_CALLBACK (ipv4_address_text_changed),
                                  editor);

        g_signal_connect_swapped (GTK_ENTRY (WID (editor->builder, "ipv4_netmask")),
                                  "notify::text-length",
                                  G_CALLBACK (ipv4_address_text_changed),
                                  editor);

        g_signal_connect_swapped (GTK_ENTRY (WID (editor->builder, "ipv4_gateway")),
                                  "notify::text-length",
                                  G_CALLBACK (ipv4_address_text_changed),
                                  editor);
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
        editor_update_proxy (editor);
        editor_update_ipv4 (editor);

        gtk_window_present (GTK_WINDOW (editor->window));

        return editor;
}
