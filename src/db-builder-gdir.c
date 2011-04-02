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

#include "db-builder.h"
#include "db-builder-gdir.h"

#include <glib.h>
#include <glib-object.h>

/*
struct DbBuilderGDirPrivate {
};
*/

static void
db_builder_gdir_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
db_builder_gdir_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static gint
add_file_to_db (const char *path,
		DMAPDb *db)
{
	return dmap_db_add_path (db, path);
}

static void
db_builder_gdir_build_db_starting_at (DbBuilder *builder, 
				      const char *dir,
				      DMAPDb *db,
				      DMAPContainerDb *container_db,
				      DMAPContainerRecord *container_record)
{
	GError *error = NULL;

	GDir *d = g_dir_open (dir, 0, &error);

	if (error != NULL) {
		g_error ("%s", error->message);
	} else {
		const gchar *entry;
		
		while ((entry = g_dir_read_name (d))) {
			gchar *path = g_strdup_printf ("%s/%s", dir, entry);

			if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
				DMAPContainerRecord *record = DMAP_CONTAINER_RECORD (g_object_new (TYPE_DMAPD_DMAP_CONTAINER_RECORD, "name", entry, "full-db", db, NULL));
				db_builder_gdir_build_db_starting_at (builder, path, db, container_db, record);
				if (dmap_container_record_get_entry_count (record) > 0)
					dmap_container_db_add (container_db, record);
				else
					g_warning ("Container %s is empty, skipping", entry);
				g_object_unref (record);
			} else {
				gchar *location;
				guint id = 0;

				location = g_strdup_printf ("file://%s", path);
				// FIXME: very expensive for BDB module:
				// id = dmap_db_lookup_id_by_location (db, location);
				g_free (location);

				if (! id) {
					id = add_file_to_db (path, db);
				} else {
					g_debug ("Loaded cached %s", path);
				}

				if (id) {
					if (container_record) {
						dmap_container_record_add_entry (container_record, NULL, id);
					}
				}
			}
			g_free (path);
		}
		
		g_dir_close (d);
	}
}

static void
db_builder_gdir_init (DbBuilderGDir *builder)
{
        /* builder->priv = DB_BUILDER_GDIR_GET_PRIVATE (builder); */
}

static void
db_builder_gdir_class_finalize (DbBuilderGDirClass *class)
{
}

static void
db_builder_gdir_finalize (GObject *object)
{
	g_debug ("Finalizing DbBuilderGDir");
}

static void db_builder_gdir_class_init (DbBuilderGDirClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	DbBuilderClass *db_builder_class = DB_BUILDER_CLASS (klass);

        /* g_type_class_add_private (klass, sizeof (DbBuilderGDirPrivate)); */

        gobject_class->set_property = db_builder_gdir_set_property;
        gobject_class->get_property = db_builder_gdir_get_property;
        gobject_class->finalize     = db_builder_gdir_finalize;

	db_builder_class->build_db_starting_at = db_builder_gdir_build_db_starting_at;
}

static void db_builder_gdir_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
        db_builder_gdir_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT void
dmapd_module_unload (GTypeModule *module)
{
}

G_DEFINE_DYNAMIC_TYPE (DbBuilderGDir,
                      db_builder_gdir,
                      TYPE_DB_BUILDER)
