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
