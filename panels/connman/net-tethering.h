/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Intel, Inc.
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

#ifndef __NET_TETHERING_H
#define __NET_TETHERING_H

#include <glib-object.h>

#include "cc-network-panel.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NET_TYPE_TETHERING         (net_tethering_get_type ())
#define NET_TETHERING(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), NET_TYPE_TETHERING, NetTethering))
#define NET_TETHERING_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), NET_TYPE_TETHERING, NetTetheringClass))
#define NET_IS_TETHERING(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), NET_TYPE_TETHERING))
#define NET_IS_TETHERING_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), NET_TYPE_TETHERING))
#define NET_TETHERING_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), NET_TYPE_TETHERING, NetTetheringClass))

typedef struct _NetTethering          NetTethering;
typedef struct _NetTetheringClass     NetTetheringClass;

struct _NetTethering
{
        GObject parent;

        GtkWidget       *parent_window;

        CcNetworkPanel  *panel;
        GtkBuilder      *builder;
        GtkWidget       *window;
};

struct _NetTetheringClass
{
        GObjectClass parent_class;

        void (*done) (NetTethering *details, gboolean success);
};

GType                net_tethering_get_type (void);
NetTethering *net_tethering_new (CcNetworkPanel *panel);
void net_tethering_setup (CcNetworkPanel *panel);

G_END_DECLS

#endif /* __NET_TETHERING_H */

