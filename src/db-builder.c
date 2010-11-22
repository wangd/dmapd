/*
 * Build a media database.
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

#include "db-builder.h"

static void
db_builder_init (DbBuilder *builder)
{
}

static void
db_builder_class_init (DbBuilderClass *klass)
{
}

G_DEFINE_TYPE (DbBuilder, db_builder, G_TYPE_OBJECT)

void
db_builder_build_db_starting_at (DbBuilder *builder,
			   const char *dir,
                           DMAPDb *db,
                           DMAPContainerDb *container_db,
                           DMAPContainerRecord *container_record)
{
	return DB_BUILDER_GET_CLASS (builder)->build_db_starting_at (builder, dir, db, container_db, container_record);
}
