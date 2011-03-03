/*
 *  Database class for DMAP sharing
 *
 * Copyright (c) 2008 W. Michael Petullo <new@flyn.org>
 * All rights reserved.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DMAPD_DMAP_DB
#define __DMAPD_DMAP_DB

#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_DMAPD_DMAP_DB           (dmapd_dmap_db_get_type ())
#define DMAPD_DMAP_DB(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_DMAPD_DMAP_DB, \
                                      DmapdDMAPDb))
#define DMAPD_DMAP_DB_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST((k), \
                                      TYPE_DMAPD_DMAP_DB, \
                                      DmapdDMAPDbClass))
#define IS_DMAPD_DMAP_DB(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_DMAPD_DMAP_DB))
#define IS_DMAPD_DMAP_DB_CLASS (k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                      TYPE_DMAPD_DMAP_DB_CLASS))
#define DMAPD_DMAP_DB_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                      TYPE_DMAPD_DMAP_DB, \
                                      DmapdDMAPDbClass))
#define DMAPD_DMAP_DB_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      TYPE_DMAPD_DMAP_DB, \
                                      DmapdDMAPDbPrivate))

typedef struct DmapdDMAPDbPrivate DmapdDMAPDbPrivate;

typedef struct {
	GObject parent;
	DmapdDMAPDbPrivate *priv;
} DmapdDMAPDb;

typedef struct {
	GObjectClass parent;

	guint (*add)                   (DMAPDb *db, DMAPRecord *record);
	guint (*add_with_id)           (DMAPDb *db,
					DMAPRecord *record,
					guint id);
	guint (*add_path)              (DMAPDb *db, const gchar *path);
	DMAPRecord *(*lookup_by_id)    (const DMAPDb *db, guint id);
	guint (*lookup_id_by_location) (const DMAPDb *db,
					const gchar *location);
	void (*foreach)                (const DMAPDb *db,
					GHFunc func,
					gpointer data);
	gint64 (*count)                (const DMAPDb *db);
} DmapdDMAPDbClass;

GType dmapd_dmap_db_get_type (void);

#endif /* __DMAPD_DMAP_DB */

G_END_DECLS
