/*   FILE: dmapd-transcode.c -- Given a file, generate the weird hash used by iTunes.
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 24 June 2013
 *
 * Copyright (c) 2013 W. Michael Petullo <new@flyn.org>
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

#include <libdmapsharing/dmap.h>
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
	guchar raw_hash[DMAP_HASH_SIZE] = { 0 };
	guchar hash[DMAP_HASH_SIZE * 2 + 1] = { 0 };
	gchar *absolute_path = NULL;

	if (argc != 2) {
		fprintf (stderr, "usage: %s path\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	if (! g_path_is_absolute (argv[1])) {
		gchar *dir = g_get_current_dir ();
		if (NULL == dir) {
			g_error ("Could not determine current directory\n");
		}

		absolute_path = g_build_filename (dir, argv[1], NULL);
	} else {
		absolute_path = g_strdup (argv[1]);
	}

	if (NULL == absolute_path) {
		g_error ("Could not build absolute path\n");
	}

	gchar *uri = g_filename_to_uri (absolute_path, NULL, NULL);
	if (NULL == uri) {
		g_error ("Could not convert %s to a URI", argv[1]);
	}

	gchar *escaped_filename = strrchr (uri, '/') + 1;
	
	if (! dmapd_util_hash_file (uri, raw_hash)) {
		exit (EXIT_FAILURE);
        }

	dmap_hash_progressive_to_string (raw_hash, hash);

	printf ("%s\n", hash);

	g_free (uri);
	g_free (absolute_path);
}
