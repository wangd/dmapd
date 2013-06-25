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
	guchar hash[33] = { 0 };

	if (argc != 2) {
		fprintf (stderr, "usage: %s path\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	gchar *uri = g_filename_to_uri (argv[1], NULL, NULL);
	gchar *escaped_filename = strrchr (uri, '/') + 1;
	dmap_hash_generate (1, (const guchar*) escaped_filename, 2, hash, 0);	
	printf ("%s\n", hash);
	g_free (uri);
}
