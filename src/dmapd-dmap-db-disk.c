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

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib/ghash.h>

#include "util.h"
#include "dmapd-dmap-db-disk.h"

/* Media ID's start at max and go down. Container ID's start at 1 and go up. */
static guint nextid = G_MAXINT; /* NOTE: this should be G_MAXUINT, but iPhoto can't handle it. */

struct DmapdDMAPDbDiskPrivate {
	GHashTable *db;
};

struct hash_id_t {
	const gchar *hash;
	guint id;
};

struct fn_data_t {
	const DMAPDb *db;
	GHFunc fn;
	gpointer user_data;
};

static GByteArray *
cache_read (const gchar *path)
{
        gchar *data;
        size_t size;
        GByteArray *blob = NULL;
        GError *error = NULL;

        g_file_get_contents (path, &data, &size, &error);
        if (error != NULL) {
                g_debug ("No record cached at %s", path);
        } else {
                blob = g_byte_array_new ();
                g_byte_array_append (blob, (guint8 *) data, size);
                g_free (data);
        }

        return blob;
}

static DMAPRecord *
load_cached_record (const DMAPDb *db, const gchar *db_dir, guchar *hash, DMAPRecordFactory *factory)
{
	DMAPRecord *record = NULL;
	gchar *path = g_strdup_printf ("%s/%s", db_dir, hash);
	if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
		GByteArray *blob = cache_read (path);
		if (blob) {
			g_debug ("Adding cache: %s", path);
			record = dmap_record_factory_create (factory, NULL);
			dmap_record_set_from_blob (record, blob);
			g_byte_array_free (blob, TRUE);
		}
	}
	g_free (path);
	return record;
}

static DMAPRecord *
dmapd_dmap_db_disk_lookup_by_id	(const DMAPDb *db, guint id)
{
	gchar *hash = NULL;
	gchar *db_dir = NULL;
	DMAPRecord *record = NULL;
	DMAPRecordFactory *factory = NULL;

	g_object_get ((gpointer) db, "record-factory", &factory, NULL);
	g_assert (factory);
	g_object_get ((gpointer) db, "db-dir", &db_dir, NULL);
	g_assert (db_dir);

	hash = g_hash_table_lookup (DMAPD_DMAP_DB_DISK (db)->priv->db, GUINT_TO_POINTER (id));
	if (hash) {
		g_debug ("Hash for %d is %s", id, hash);
		record = load_cached_record (db, db_dir, hash, factory);
		if (! record) {
			g_warning ("Record %s not found", hash);
		}
	} else {
		g_warning ("Record %d not found", id);
	}

        return record;
}

static gboolean
hash_match (gpointer key, gpointer val, gpointer user_data)
{
	struct hash_id_t *hash_id = user_data;
	hash_id->id = GPOINTER_TO_UINT (key);
	return ! strcmp (val, hash_id->hash); 
}

static guint
dmapd_dmap_db_disk_lookup_id_by_location (const DMAPDb *db, const gchar *location)
{
	guint fnval;
	guchar hash[33];
	struct hash_id_t user_data;

	hash[32] = 0x00;
	dmap_hash_generate (1, (const guchar*) location, 2, hash, 0);

	user_data.hash = location;
	user_data.id = 0;

	if (g_hash_table_find (DMAPD_DMAP_DB_DISK (db)->priv->db,
		(GHRFunc) hash_match,
		(gpointer) &user_data)) {
		fnval = user_data.id;
	} else {
		fnval = 0;
	}

	return fnval;
}

static void
dmapd_dmap_db_disk_foreach_ghfunc (gpointer key, gpointer value, gpointer user_data)
{

	DMAPRecord *record;
	struct fn_data_t *fn_data = user_data;

	g_debug ("Processing id %u", GPOINTER_TO_UINT (key));

	record = dmapd_dmap_db_disk_lookup_by_id(fn_data->db, GPOINTER_TO_UINT (key));
	if (record) {
		fn_data->fn (key, record, fn_data->user_data);
		g_object_unref (record);
	} else {
		g_warning ("Record %u not found", GPOINTER_TO_UINT (key));
	}
}

static void
dmapd_dmap_db_disk_foreach	(const DMAPDb *db,
				 GHFunc func,
				 gpointer data)
{
	struct fn_data_t user_data;
	user_data.db = db;
	user_data.fn = func;
	user_data.user_data = data;
	g_hash_table_foreach (DMAPD_DMAP_DB_DISK (db)->priv->db, dmapd_dmap_db_disk_foreach_ghfunc, &user_data);
}

static gint64
dmapd_dmap_db_disk_count (const DMAPDb *db)
{
	return g_hash_table_size (DMAPD_DMAP_DB_DISK (db)->priv->db);
}

static gchar *
cache_path (const gchar *db_dir, const gchar *hash)
{
        gchar *cachepath = NULL;

        cachepath = g_strdup_printf ("%s/%s", db_dir, hash);

        return cachepath;
}

static void
cache_store (const gchar *db_dir, const gchar *hash, GByteArray *blob)
{
        struct stat st;
        gchar *cachepath;
        GError *error = NULL;
        /* NOTE: g_stat seemed broken; would corrupt GError *error. */
        if (stat (db_dir, &st) != 0) {
                g_warning ("cache directory %s does not exist, will not cache", db_dir);
                return;
        }
        if (! (st.st_mode & S_IFDIR)) {
                g_warning ("%s is not a directory, will not cache", db_dir);
                return;
        }
        cachepath = cache_path (db_dir, hash);
        g_file_set_contents (cachepath,
			    (gchar *) blob->data,
			     blob->len,
			     &error);
        if (error != NULL) {
                g_warning ("Error writing %s", cachepath);
        }

        g_free (cachepath);
}

static guint
dmapd_dmap_db_disk_add_with_id (DMAPDb *db, DMAPRecord *record, guint id)
{
	gchar *location;
	GByteArray *blob;
	gchar *db_dir = NULL;
	guchar *hash;

	g_object_get (record, "location", &location, NULL);
	g_assert (location);
	g_object_get (db, "db-dir", &db_dir, NULL);
	if (! db_dir) {
		g_error ("Database directory not set");
	}

	hash = g_new (guchar, 33);
	hash[32] = 0x00;

        dmap_hash_generate (1, (const guchar*) location, 2, hash, 0);

	blob = dmap_record_to_blob (record);
	cache_store (db_dir, hash, blob);
	g_free (location);
	g_byte_array_free (blob, TRUE);
	g_hash_table_insert (DMAPD_DMAP_DB_DISK (db)->priv->db, GUINT_TO_POINTER (id), hash);

	return id;
}

static guint
dmapd_dmap_db_disk_add (DMAPDb *db, DMAPRecord *record)
{
	return dmapd_dmap_db_disk_add_with_id (db, record, nextid--);
}

static guint
dmapd_dmap_db_disk_add_path (DMAPDb *db, const gchar *path)
{
	guint id;
	DMAPRecord *record;
	DMAPRecordFactory *factory = NULL;

	g_object_get (db, "record-factory", &factory, NULL);
	g_assert (factory);
	record = dmap_record_factory_create (factory, (gpointer) path);

	if (record) {
		id = dmapd_dmap_db_disk_add (db, record);
		g_object_unref (record);
	} else {
		id = 0;
	}

	return id;
}

G_DEFINE_DYNAMIC_TYPE (DmapdDMAPDbDisk,
		       dmapd_dmap_db_disk,
		       TYPE_DMAPD_DMAP_DB)

static void dmapd_dmap_db_disk_init (DmapdDMAPDbDisk *db)
{
	db->priv = DMAPD_DMAP_DB_DISK_GET_PRIVATE (db);
	db->priv->db = g_hash_table_new_full (g_direct_hash,
					      g_direct_equal,
					      NULL,
					      g_object_unref);
}

static void
dmapd_dmap_db_disk_finalize (GObject *object)
{
	DmapdDMAPDbDisk *db = DMAPD_DMAP_DB_DISK (object);

	g_debug ("Finalizing DmapdDMAPDbDisk (%d records)",
		 g_hash_table_size (db->priv->db));

	g_hash_table_destroy (db->priv->db);
}

static void
dmapd_dmap_db_disk_class_finalize (DmapdDMAPDbDiskClass *object)
{
}

static void dmapd_dmap_db_disk_class_init (DmapdDMAPDbDiskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	DmapdDMAPDbClass *dmap_db_class = DMAPD_DMAP_DB_CLASS (klass);

	object_class->finalize = dmapd_dmap_db_disk_finalize;

	dmap_db_class->add = dmapd_dmap_db_disk_add;
	dmap_db_class->add_with_id = dmapd_dmap_db_disk_add_with_id;
	dmap_db_class->add_path = dmapd_dmap_db_disk_add_path;
	dmap_db_class->lookup_by_id = dmapd_dmap_db_disk_lookup_by_id;
	dmap_db_class->lookup_id_by_location = dmapd_dmap_db_disk_lookup_id_by_location;
	dmap_db_class->foreach = dmapd_dmap_db_disk_foreach;
	dmap_db_class->count = dmapd_dmap_db_disk_count;

	g_type_class_add_private (klass, sizeof (DmapdDMAPDbDiskPrivate));
}

static void dmapd_dmap_db_disk_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
	dmapd_dmap_db_disk_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule *module)
{
	return TRUE;
}
