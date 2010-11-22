/*
 * Database record class for DMAP sharing
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

#ifndef __DMAPD_DMAP_CONTAINER_RECORD
#define __DMAPD_DMAP_CONTAINER_RECORD

#include <libdmapsharing/dmap.h>

#include "dmapd-dmap-db-ghashtable.h"
#include "dmapd-dmap-db-bdb.h"

G_BEGIN_DECLS

#define TYPE_DMAPD_DMAP_CONTAINER_RECORD         (dmapd_dmap_container_record_get_type ())
#define DMAPD_DMAP_CONTAINER_RECORD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), \
				       TYPE_DMAPD_DMAP_CONTAINER_RECORD, DmapdDMAPContainerRecord))
#define DMAPD_DMAP_CONTAINER_RECORD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), \
			               TYPE_DMAPD_DMAP_CONTAINER_RECORD, \
				       DmapdDMAPContainerRecordClass))
#define IS_DMAPD_DMAP_CONTAINER_RECORD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
				       TYPE_DMAPD_DMAP_CONTAINER_RECORD))
#define IS_DMAPD_DMAP_CONTAINER_RECORD_CLASS (k) (G_TYPE_CHECK_CLASS_TYPE ((k), \
				       TYPE_DMAPD_DMAP_CONTAINER_RECORD_CLASS))
#define DMAPD_DMAP_CONTAINER_RECORD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), \
				       TYPE_DMAPD_DMAP_CONTAINER_RECORD, \
				       DmapdDMAPContainerRecordClass))
#define DMAPD_DMAP_CONTAINER_RECORD_GET_PRIVATE(o)	     (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					      TYPE_DMAPD_DMAP_CONTAINER_RECORD, \
					      DmapdDMAPContainerRecordPrivate))

typedef struct DmapdDMAPContainerRecordPrivate DmapdDMAPContainerRecordPrivate;

typedef struct {
	GObject parent;
	DmapdDMAPContainerRecordPrivate *priv;
} DmapdDMAPContainerRecord;

typedef struct {
	GObjectClass parent;
} DmapdDMAPContainerRecordClass;

GType dmapd_dmap_container_record_get_type (void);

#endif /* __DMAPD_DMAP_CONTAINER_RECORD */

G_END_DECLS
