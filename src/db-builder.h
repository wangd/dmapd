/*
 * Build a media database.
 *
 * Copyright (c) 2009 W. Michael Petullo <new@flyn.org>
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

#ifndef __DB_BUILDER
#define __DB_BUILDER

#include <glib.h>

#include <libdmapsharing/dmap.h>

G_BEGIN_DECLS

#define TYPE_DB_BUILDER          (db_builder_get_type ())
#define DB_BUILDER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                                  TYPE_DB_BUILDER, DbBuilder))
#define DB_BUILDER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), \
                                  TYPE_DB_BUILDER, DbBuilderClass))
#define IS_DB_BUILDER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                                  TYPE_DB_BUILDER))
#define IS_DB_BUILDER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), \
                                  TYPE_DB_BUILDER))
#define DB_BUILDER_GET_CLASS(k)  (G_TYPE_INSTANCE_GET_CLASS ((k), \
                                  TYPE_DB_BUILDER, DbBuilderClass))

typedef struct _DbBuilder DbBuilder;
typedef struct _DbBuilderClass DbBuilderClass;

struct _DbBuilder {
        GObject parent;
};

struct _DbBuilderClass {
        GObjectClass parent;

	void (*build_db_starting_at) (DbBuilder *builder,
                                      const char *dir,
                                      DMAPDb *db,
                                      DMAPContainerDb *container_db,
                                      DMAPContainerRecord *container_record);
};

GType       db_builder_get_type      (void);

void db_builder_build_db_starting_at (DbBuilder *builder,
				      const char *dir,
				      DMAPDb *db,
				      DMAPContainerDb *container_db,
				      DMAPContainerRecord *container_record);

#endif /* __DB_BUILDER */

G_END_DECLS
