/*
 * Build a media database from filesystem content.
 *
 * Copyright (C) 2009 W. Michael Petullo <mike@flyn.org>
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

#ifndef __DB_BUILDER_GDIR
#define __DB_BUILDER_GDIR

#include <glib.h>

#include "db-builder.h"
#include "dmapd-dmap-container-record.h"
#include "dmapd-dmap-container-db.h"
#include "dmapd-dpap-record-factory.h"
#include "dmapd-daap-record.h"
#include "dmapd-daap-record-factory.h"

G_BEGIN_DECLS

#define TYPE_DB_BUILDER_GDIR           (db_builder_gdir_get_type ())
#define DB_BUILDER_GDIR(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                        TYPE_DB_BUILDER_GDIR, DbBuilderGDir))
#define DB_BUILDER_GDIR_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST (k), \
				        TYPE_DB_BUILDER_GDIR, \
                                        DbBuilderGDirClass)
#define IS_DB_BUILDER_GDIR(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                        TYPE_DB_BUILDER_GDIR))
#define IS_DB_BUILDER_GDIR_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), \
				        TYPE_DB_BUILDER_GDIR_CLASS)) 
#define DB_BUILDER_GDIR_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS ((o), \
                                        TYPE_DB_BUILDER_GDIR, \
				        DbBuilderGDirInterfaceClass))
#define DB_BUILDER_GDIR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					TYPE_DB_BUILDER_GDIR, \
					DbBuilderGDirPrivate))

typedef struct DbBuilderGDirPrivate DbBuilderGDirPrivate;

typedef struct {
	DbBuilder parent;
	DbBuilderGDirPrivate *priv;
} DbBuilderGDir;

typedef struct {
	DbBuilderClass parent;
} DbBuilderGDirClass;

GType       db_builder_gdir_get_type      (void);

#endif /* __DB_BUILDER_GDIR */

G_END_DECLS
