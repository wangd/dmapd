/*   FILE: util.h -- utility functions
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 01 January 2009 
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

#ifndef __DMAPD_UTIL
#define __DMAPD_UTIL

#include <glib.h>
#include <glib-object.h>

typedef enum {
	CACHE_TYPE_RECORD,
	CACHE_TYPE_TRANSCODED_DATA,
	CACHE_TYPE_THUMBNAIL_DATA
} cache_type_t;

gchar *parse_plugin_option (gchar *str, GHashTable *hash_table);

GByteArray *blob_add_atomic (GByteArray *blob,
			     const guint8 *ptr,
			     const size_t size);

GByteArray *blob_add_string (GByteArray *blob, const gchar *str);

void slist_deep_free (GSList *list);

void stringleton_init (void);

const gchar *stringleton_ref (const gchar *str);

void stringleton_unref (const gchar *str);

void stringleton_deinit (void);

GObject *object_from_module (GType type, const gchar *module_name, const gchar *first_property_name, ...);

gchar *cache_path (cache_type_t type, const gchar *db_dir, const gchar *imagepath);

#endif
