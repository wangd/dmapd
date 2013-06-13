/*   FILE: util-gst.c -- GStreamer utility functions
 * AUTHOR: W. Michael Petullo <mike@flyn.org>
 *   DATE: 11 May 2011 
 *
 * Copyright (c) 2011 W. Michael Petullo <new@flyn.org>
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

#include <stdio.h>

#include "util.h"
#include "util-gst.h"

/* FIXME: copied from libdmapsharing: */
gboolean
pads_compatible (GstPad *pad1, GstPad *pad2)
{
        gboolean fnval;
        GstCaps *res, *caps1, *caps2;

        caps1 = gst_pad_query_caps (pad1, NULL);
        caps2 = gst_pad_query_caps (pad2, NULL);
        res = gst_caps_intersect (caps1, caps2);
        fnval = res && ! gst_caps_is_empty (res);

        gst_caps_unref (res);
        gst_caps_unref (caps2);
        gst_caps_unref (caps1);

        return fnval;
}

gboolean
transition_pipeline (GstElement *pipeline, GstState state)
{
	gboolean fnval = TRUE;
	GstStateChangeReturn sret;

	sret = gst_element_set_state (GST_ELEMENT (pipeline), state);
	if (GST_STATE_CHANGE_ASYNC == sret) {
		if (GST_STATE_CHANGE_SUCCESS != gst_element_get_state
				(GST_ELEMENT (pipeline),
				 &state,
				 NULL,
				 1 * GST_SECOND)) {
			g_warning ("State change failed");
			fnval = FALSE;
		}
	} else if (sret != GST_STATE_CHANGE_SUCCESS) {
		g_warning ("Could not read file");
		fnval = FALSE;
	}
	return fnval;
}

static void
do_transcode (DAAPRecord *record, gchar *cachepath)
{
	GError *error = NULL;
	GInputStream *stream, *decoded_stream;
	
	stream = daap_record_read (record, &error);
	if (! stream) {
		gchar *location = NULL;
		g_object_get (record, "location", &location, NULL);
		g_warning ("Error opening %s: %s", location, error->message);
		g_error_free (error);
		goto _return_no_close;
	}
	/* FIXME: make target format flexible: */
	decoded_stream = dmap_gst_input_stream_new ("audio/mp3", stream);
	if (! decoded_stream) {
		gchar *location;
		g_object_get (record, "location", &location, NULL);
		g_warning ("Error opening %s", location);
		g_error_free (error);
		goto _return_no_close_decoded_stream;
	}
	gssize read_size;
	gchar buf[BUFSIZ];

	FILE *outfile = fopen (cachepath, "w");
	if (outfile == NULL) {
		 g_warning ("Error opening: %s", cachepath);
		 goto _return_no_close_outfile;
	}

	/* FIXME: is there a glib function to do this? */
	do {
		read_size = g_input_stream_read (decoded_stream,
						 buf,
						 BUFSIZ,
						 NULL,
						&error);
		if (read_size > 0) {
			if (fwrite (buf, 1, read_size, outfile) != read_size) {
				 g_warning ("Error writing transcoded data");
				 goto _return;
			}
		} else if (error != NULL) {
			g_warning ("Error transcoding: %s", error->message);
			g_error_free (error);
			goto _return;
		}
	} while (read_size > 0);

_return:
	fclose (outfile);
_return_no_close_outfile:
	g_input_stream_close (decoded_stream, NULL, NULL);
_return_no_close_decoded_stream:
	g_input_stream_close (stream, NULL, NULL); /* FIXME: should this be done in GGstMp3InputStream class? */
_return_no_close:
	return;
}

/* NOTE: This is here and not in the individual DMAPRecords because records
 * have no knowlege of the database, db_dir, etc.
 */
void
transcode_cache (gpointer id, DAAPRecord *record, gchar *db_dir)
{
	gboolean has_video = FALSE;
	gchar *location = NULL;
	gchar *format = NULL;
	gchar *cacheuri = NULL;
	gchar *cachepath = NULL;

	g_object_get (record,
		     "location",
		     &location,
		      "format",
		     &format,
		     "has-video",
		     &has_video,
		      NULL);

	if (! (location && format)) {
		g_warning ("Error reading record properties for transcoding");
		return;
	}

	/* FIXME: make target format flexible: */
	if (! strcmp (format, "mp3")) {
		g_debug ("Transcoding not necessary %s", location);
		return;
	}

	if (has_video) {
		g_debug ("Not transcoding video %s", location);
		return;
	}

	g_assert (db_dir);
	cachepath = cache_path (CACHE_TYPE_TRANSCODED_DATA, db_dir, location);

	if (! g_file_test (cachepath, G_FILE_TEST_EXISTS)) {
		/* FIXME: return value, not void: */
		g_debug ("Transcoding %s to %s", location, cachepath);
		do_transcode (record, cachepath);
	} else {
		g_debug ("Found transcoded data at %s for %s", cachepath, location);
	}

	/* Replace previous location with URI to transcoded file. */
	cacheuri = g_filename_to_uri(cachepath, NULL, NULL);
	g_object_set (record, "location", cacheuri, NULL);
	g_free (cacheuri);
	/* FIXME: make target format flexible: */
	g_object_set (record, "format", "mp3", NULL);

	g_free (cachepath);

	return;
}
