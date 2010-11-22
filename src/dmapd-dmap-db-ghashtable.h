/*
 *  Database class for DMAP sharing
 *
 *  Copyright (C) 2008 W. Michael Petullo <mike@flyn.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __DMAPD_DMAP_DB_GHASHTABLE
#define __DMAPD_DMAP_DB_GHASHTABLE

#include <libdmapsharing/dmap.h>

#include "dmapd-dmap-db.h"

G_BEGIN_DECLS

#define TYPE_DMAPD_DMAP_DB_GHASHTABLE           (dmapd_dmap_db_ghashtable_get_type ())
#define DMAPD_DMAP_DB_GHASHTABLE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                      TYPE_DMAPD_DMAP_DB_GHASHTABLE, \
                                      DmapdDMAPDbGHashTable))
#define DMAPD_DMAP_DB_GHASHTABLE_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST((k), \
                                      TYPE_DMAPD_DMAP_DB_GHASHTABLE, \
                                      DmapdDMAPDbGHashTableClass))
#define IS_DMAPD_DMAP_DB_GHASHTABLE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                      TYPE_DMAPD_DMAP_DB_GHASHTABLE))
#define IS_DMAPD_DMAP_DB_GHASHTABLE_CLASS (k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                      TYPE_DMAPD_DMAP_DB_GHASHTABLE_CLASS))
#define DMAPD_DMAP_DB_GHASHTABLE_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                      TYPE_DMAPD_DMAP_DB_GHASHTABLE, \
                                      DmapdDMAPDbGHashTableClass))
#define DMAPD_DMAP_DB_GHASHTABLE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                      TYPE_DMAPD_DMAP_DB_GHASHTABLE, \
                                      DmapdDMAPDbGHashTablePrivate))

typedef struct DmapdDMAPDbGHashTablePrivate DmapdDMAPDbGHashTablePrivate;

typedef struct {
	GObject parent;
	DmapdDMAPDbGHashTablePrivate *priv;
} DmapdDMAPDbGHashTable;

typedef struct {
	GObjectClass parent;
} DmapdDMAPDbGHashTableClass;

GType dmapd_dmap_db_ghashtable_get_type (void);

#endif /* __DMAPD_DMAP_DB_GHASHTABLE */

G_END_DECLS
