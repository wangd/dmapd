/*
 * Database class for DMAP sharing
 *
 * Copyright (C) 2008 W. Michael Petullo <mike@flyn.org>
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
 */

#ifndef __DMAPD_DMAP_CONTAINER_DB
#define __DMAPD_DMAP_CONTAINER_DB

#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_DMAPD_DMAP_CONTAINER_DB         (dmapd_dmap_container_db_get_type ())
#define DMAPD_DMAP_CONTAINER_DB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), \
				   TYPE_DMAPD_DMAP_CONTAINER_DB, DmapdDMAPContainerDb))
#define DMAPD_DMAP_CONTAINER_DB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), \
				   TYPE_DMAPD_DMAP_CONTAINER_DB, DmapdDMAPContainerDbClass))
#define IS_DMAPD_DMAP_CONTAINER_DB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
				   TYPE_DMAPD_DMAP_CONTAINER_DB))
#define IS_DMAPD_DMAP_CONTAINER_DB_CLASS (k) (G_TYPE_CHECK_CLASS_TYPE ((k), \
				   TYPE_DMAPD_DMAP_CONTAINER_DB_CLASS))
#define DMAPD_DMAP_CONTAINER_DB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
				   TYPE_DMAPD_DMAP_CONTAINER_DB, DmapdDMAPContainerDbClass))
#define DMAPD_DMAP_CONTAINER_DB_GET_PRIVATE(o)	     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					      TYPE_DMAPD_DMAP_CONTAINER_DB, \
					      DmapdDMAPContainerDbPrivate))

typedef struct DmapdDMAPContainerDbPrivate DmapdDMAPContainerDbPrivate;

typedef struct {
	GObject parent;
	DmapdDMAPContainerDbPrivate *priv;
} DmapdDMAPContainerDb;

typedef struct {
	GObjectClass parent;
} DmapdDMAPContainerDbClass;

DmapdDMAPContainerDb *dmapd_dmap_container_db_new             (void);

GType       dmapd_dmap_container_db_get_type        (void);

DMAPContainerRecord *dmapd_dmap_container_db_lookup_by_id    (DMAPContainerDb *db, guint id);

void        dmapd_dmap_container_db_foreach (DMAPContainerDb *db,
					  void (*fn) (gpointer key,
					 	      gpointer value,
						      gpointer user_data),
						      gpointer data);

gint64      dmapd_dmap_container_db_count (DMAPContainerDb *db);

void dmapd_dmap_container_db_add (DMAPContainerDb *db, DMAPContainerRecord *record);

#endif /* __DMAPD_DMAP_CONTAINER_DB */

G_END_DECLS
