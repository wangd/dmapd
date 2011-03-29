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
#include "dmapd-dmap-db-ghashtable.h"

/* Media ID's start at max and go down. Container ID's start at 1 and go up. */
static guint nextid = G_MAXINT; /* NOTE: this should be G_MAXUINT, but iPhoto can't handle it. */

struct DmapdDMAPDbGHashTablePrivate {
	GHashTable *db;
	gchar *db_dir;
	DMAPRecordFactory *record_factory;
};

enum {
        PROP_0,
	PROP_DB_DIR,
	PROP_RECORD_FACTORY
};

struct loc_id {
	const gchar *location;
	guint id;
};

static gboolean
path_match (gpointer key, gpointer val, gpointer user_data)
{
	gboolean fnval;
	gchar *location;

	((struct loc_id *) user_data)->id = GPOINTER_TO_UINT (key);
	g_object_get (DMAP_RECORD (val), "location", &location, NULL);

	fnval = ! strcmp (location, ((struct loc_id *) user_data)->location); 

	g_free (location);

	return fnval;
}

static guint
dmapd_dmap_db_ghashtable_lookup_id_by_location (const DMAPDb *db, const gchar *location)
{
	guint fnval;
	struct loc_id user_data;

	user_data.location = location;
	user_data.id = 0;

	if (g_hash_table_find (DMAPD_DMAP_DB_GHASHTABLE (db)->priv->db,
		(GHRFunc) path_match,
		(gpointer) &user_data)) {
		fnval = user_data.id;
	} else {
		fnval = 0;
	}

	return fnval;
}

static void
dmapd_dmap_db_ghashtable_stash_thumbnail (DMAPDb *db, DMAPRecord *record)
{
	GByteArray *thumbnail = NULL;
	gchar *cachepath = NULL, *location = NULL, *db_dir = NULL;

	g_object_get (db,
	             "db-dir",
		     &db_dir,
		      NULL);

	g_object_get (record,
	             "location",
		     &location,
	             "thumbnail",
		     &thumbnail,
		      NULL);

	if (db_dir && location && thumbnail && thumbnail->len > 0) {
		GError *error = NULL;

		g_debug ("Writing thumbnail to cache");

		cachepath = cache_path (CACHE_TYPE_THUMBNAIL_DATA, db_dir, location);

		g_assert (cachepath);

		/* Note that this overwrites existing thumbnail cache which is fine. */
		g_file_set_contents (cachepath,
			    (gchar *) thumbnail->data,
			     thumbnail->len,
			    &error);

		if (error) {
			g_warning ("Error writing thumbnail to %s", cachepath);
		} else {
			thumbnail = g_byte_array_sized_new (0);
			g_object_set (record, "thumbnail", thumbnail, NULL);
			g_byte_array_unref(thumbnail);
		}

		g_free (cachepath);
	}

	if (db_dir)
		g_free (db_dir);
	if (location)
		g_free (location);
}

static void
dmapd_dmap_db_ghashtable_unstash_thumbnail (DMAPDb *db, DMAPRecord *record)
{
	gchar *location = NULL, *db_dir = NULL;
	GByteArray *thumbnail = NULL;

	g_object_get (db,
	             "db-dir",
		     &db_dir,
		      NULL);

	g_object_get (record,
	             "location",
		     &location,
	             "thumbnail",
		     &thumbnail,
		      NULL);

	if (db_dir && location && thumbnail && thumbnail->len == 0) {
		size_t  size;
		GError *error = NULL;
		gchar *data = NULL, *cachepath = NULL;

		g_debug ("Thumbnail size 0, looking for cached thumbnail");

		cachepath = cache_path (CACHE_TYPE_THUMBNAIL_DATA, db_dir, location);

		g_assert (cachepath);

		g_file_get_contents (cachepath, &data, &size, &error);
		if (error != NULL) {
			g_debug ("No thumbnail cached at %s", cachepath);
		} else {
			thumbnail = g_byte_array_sized_new (size);
			g_byte_array_append (thumbnail, data, size);
			g_object_set (record, "thumbnail", thumbnail, NULL);
			g_byte_array_unref(thumbnail);
			g_free (data);
		}

		g_free (cachepath);
	}

	if (db_dir)
		g_free (db_dir);
	if (location)
		g_free (location);
}

static DMAPRecord *
dmapd_dmap_db_ghashtable_lookup_by_id	(const DMAPDb *db, guint id)
{
	DMAPRecord *record;

	record = g_hash_table_lookup (DMAPD_DMAP_DB_GHASHTABLE (db)->priv->db, GUINT_TO_POINTER (id));
	g_object_ref (record);

	/* Kludge to avoid keeping thumbnails in memory: */
	if (IS_DPAP_RECORD (record)) {
		dmapd_dmap_db_ghashtable_unstash_thumbnail (db, record);
	}

	return record;
}

static void
dmapd_dmap_db_ghashtable_foreach	(const DMAPDb *db,
				 GHFunc func,
				 gpointer data)
{
	g_hash_table_foreach (DMAPD_DMAP_DB_GHASHTABLE (db)->priv->db, func, data);
}

static gint64
dmapd_dmap_db_ghashtable_count (const DMAPDb *db)
{
	return g_hash_table_size (DMAPD_DMAP_DB_GHASHTABLE (db)->priv->db);
}

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

static void
load_cached_records (DMAPDb *db, const gchar *db_dir, DMAPRecordFactory *factory)
{
	GDir *d;
	GError *error = NULL;

	d = g_dir_open (db_dir, 0, &error);

	if (error != NULL) {
		g_warning ("%s", error->message);
	} else {
		if (db_dir) {
			const gchar *entry;

			while ((entry = g_dir_read_name (d))) {
				gchar *path = g_strdup_printf ("%s/%s", db_dir, entry);
				if (g_file_test (path, G_FILE_TEST_IS_REGULAR)
				    && g_str_has_suffix (path, ".record")) {
					GByteArray *blob = cache_read (path);
					if (blob) {
						g_debug ("Adding cache: %s", path);
						DMAPRecord *record = dmap_record_factory_create (factory, NULL);
						dmap_record_set_from_blob (record, blob);
						dmap_db_add (DMAP_DB (db), record);
						g_byte_array_free (blob, TRUE);
						g_object_unref (record);
					}
				}
				g_free (path);
			}
		}

		g_dir_close (d);
	}
}

static guint
dmapd_dmap_db_ghashtable_add_with_id (DMAPDb *db, DMAPRecord *record, guint id)
{
	/* Kludge to avoid keeping thumbnails in memory: */
	if (IS_DPAP_RECORD (record)) {
		dmapd_dmap_db_ghashtable_stash_thumbnail (db, record);
	}

	g_hash_table_insert (DMAPD_DMAP_DB_GHASHTABLE (db)->priv->db, GUINT_TO_POINTER (id), record);
	return id;
}

static void
cache_store (const gchar *db_dir, const gchar *imagepath, GByteArray *blob)
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
        cachepath = cache_path (CACHE_TYPE_RECORD, db_dir, imagepath);
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
dmapd_dmap_db_ghashtable_add (DMAPDb *db, DMAPRecord *record)
{
	gchar *db_dir = NULL;
	GByteArray *blob;
	gchar *location;

	g_object_ref (record);

	blob = dmap_record_to_blob (record);
	g_object_get (record, "location", &location, NULL);
	g_object_get (db, "db-dir", &db_dir, NULL);
	if (db_dir != NULL)
		cache_store (db_dir, location, blob);
	g_free (location);
	g_free (db_dir);
	g_byte_array_free (blob, TRUE);

	return dmapd_dmap_db_ghashtable_add_with_id (db, record, nextid--);
}

static guint
dmapd_dmap_db_ghashtable_add_path (DMAPDb *db, const gchar *path)
{
	guint id;
	DMAPRecord *record;
	DMAPRecordFactory *factory = NULL;

	g_object_get (db, "record-factory", &factory, NULL);
	g_assert (factory);
	record = dmap_record_factory_create (factory, (gpointer) path);

	if (record) {
		id = dmapd_dmap_db_ghashtable_add (db, record);
		g_object_unref (record);
	} else {
		id = 0;
	}

	return id;
}

static void dmapd_dmap_ghashtable_interface_init (gpointer iface, gpointer data)
{
        DMAPDbIface *dmap_db = iface;

	g_assert (G_TYPE_FROM_INTERFACE (dmap_db) == DMAP_TYPE_DB);

	dmap_db->add = dmapd_dmap_db_ghashtable_add;
	dmap_db->add_with_id = dmapd_dmap_db_ghashtable_add_with_id;
	dmap_db->add_path = dmapd_dmap_db_ghashtable_add_path;
	dmap_db->lookup_by_id = dmapd_dmap_db_ghashtable_lookup_by_id;
	dmap_db->lookup_id_by_location = dmapd_dmap_db_ghashtable_lookup_id_by_location;
	dmap_db->foreach = dmapd_dmap_db_ghashtable_foreach;
	dmap_db->count = dmapd_dmap_db_ghashtable_count;
}

G_DEFINE_TYPE_WITH_CODE (DmapdDMAPDbGHashTable, dmapd_dmap_db_ghashtable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (DMAP_TYPE_DB, dmapd_dmap_ghashtable_interface_init))

static GObject*
dmapd_dmap_db_ghashtable_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;
	gchar *db_dir = NULL;
	DMAPRecordFactory *factory = NULL;
  
	object = G_OBJECT_CLASS (dmapd_dmap_db_ghashtable_parent_class)->constructor (type, n_construct_params, construct_params);

	g_object_get (object, "db-dir", &db_dir, "record-factory", &factory, NULL);
	/* NOTE: Don't load cache when used for DmapdDMAPContainerRecord: */
	if (db_dir && factory) {
		load_cached_records (DMAP_DB (object), db_dir, factory);
	}
	g_free (db_dir);

	return object;
}

static void dmapd_dmap_db_ghashtable_init (DmapdDMAPDbGHashTable *db)
{
	db->priv = DMAPD_DMAP_DB_GHASHTABLE_GET_PRIVATE (db);
	db->priv->db = g_hash_table_new_full (g_direct_hash,
					      g_direct_equal,
					      NULL,
					      g_object_unref);
}

static void
dmapd_dmap_db_ghashtable_finalize (GObject *object)
{
	DmapdDMAPDbGHashTable *db = DMAPD_DMAP_DB_GHASHTABLE (object);

	g_debug ("Finalizing DmapdDMAPDbGHashTable (%d records)",
		 g_hash_table_size (db->priv->db));

	g_hash_table_destroy (db->priv->db);
}

static void
dmapd_dmap_db_ghashtable_set_property (GObject *object,
                            guint prop_id,
			    const GValue *value,
			    GParamSpec *pspec)
{
        DmapdDMAPDbGHashTable *db = DMAPD_DMAP_DB_GHASHTABLE (object);

	switch (prop_id) {
		case PROP_DB_DIR:
			g_free (db->priv->db_dir);
			db->priv->db_dir = g_value_dup_string (value);
			break;
		case PROP_RECORD_FACTORY:
			if (db->priv->record_factory)
				g_object_unref (db->priv->record_factory);
			db->priv->record_factory = DMAP_RECORD_FACTORY (g_value_get_pointer (value));
			break;	
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
dmapd_dmap_db_ghashtable_get_property (GObject *object,
                            guint prop_id,
			    GValue *value,
			    GParamSpec *pspec)
{
        DmapdDMAPDbGHashTable *db = DMAPD_DMAP_DB_GHASHTABLE (object);

	switch (prop_id) {
		case PROP_DB_DIR:
			g_value_set_static_string (value, db->priv->db_dir);
			break;
		case PROP_RECORD_FACTORY:
			g_value_set_pointer (value, db->priv->record_factory);
			break;	
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}


static void dmapd_dmap_db_ghashtable_class_init (DmapdDMAPDbGHashTableClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DmapdDMAPDbGHashTablePrivate));

	object_class->finalize = dmapd_dmap_db_ghashtable_finalize;
	object_class->constructor = dmapd_dmap_db_ghashtable_constructor;
	object_class->set_property = dmapd_dmap_db_ghashtable_set_property;
	object_class->get_property = dmapd_dmap_db_ghashtable_get_property;

	g_object_class_install_property (object_class,
					 PROP_RECORD_FACTORY,
					 g_param_spec_pointer ("record-factory",
					 		       "Record factory",
							       "Record factory",
							        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_object_class_install_property (object_class,
					 PROP_DB_DIR,
					 g_param_spec_string ("db-dir",
					 		      "Directory for database cache",
							      "Directory for database cache",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}
