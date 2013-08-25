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

#include <libdmapsharing/dmap.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "util.h"
#include "util-gst.h"

/* FIXME: copied from libdmapsharing: */
gboolean
pads_compatible (GstPad *pad1, GstPad *pad2)
{
        gboolean fnval = FALSE;
        GstCaps *res, *caps1, *caps2;

        caps1 = gst_pad_query_caps (pad1, NULL);
        caps2 = gst_pad_query_caps (pad2, NULL);

	if (NULL == caps1 || NULL == caps2) {
		g_warning ("Could not get caps from pad");
		goto done;
	}

        res = gst_caps_intersect (caps1, caps2);
	if (NULL == res) {
		g_warning ("Could not get res from caps");
		goto done;
	}

        fnval = res && ! gst_caps_is_empty (res);

done:
	if (NULL != res) {
		gst_caps_unref (res);
	}

	if (NULL != caps1) {
		gst_caps_unref (caps1);
	}

	if (NULL != caps2) {
		gst_caps_unref (caps2);
	}

        return fnval;
}

gboolean
transition_pipeline (GstElement *pipeline, GstState state)
{
	gboolean fnval = TRUE;
	GstStateChangeReturn sret;

	sret = gst_element_set_state (GST_ELEMENT (pipeline), state);
	if (GST_STATE_CHANGE_ASYNC == sret) {
		if (GST_STATE_CHANGE_SUCCESS != gst_element_get_state (GST_ELEMENT (pipeline), &state, NULL, GST_CLOCK_TIME_NONE)) {
			g_warning ("Asynchronous state change failed");
			fnval = FALSE;
		}
	} else if (sret != GST_STATE_CHANGE_SUCCESS) {
		g_warning ("State change failed.");
		fnval = FALSE;
	}
	return fnval;
}

static void
do_transcode (DAAPRecord *record, gchar *cachepath, gchar* target_mimetype)
{
	gssize read_size;
	gchar buf[BUFSIZ];
	GError *error = NULL;
	GInputStream *stream = NULL;
	GInputStream *decoded_stream = NULL;
	
	stream = daap_record_read (record, &error);
	if (NULL == stream) {
		gchar *location = NULL;
		g_object_get (record, "location", &location, NULL);
		g_assert (NULL != location);
		g_warning ("Error opening %s: %s", location, error->message);
		g_error_free (error);
		g_free (location);
		goto _return;
	}
	decoded_stream = dmap_gst_input_stream_new (target_mimetype, stream);
	if (NULL == decoded_stream) {
		gchar *location;
		g_object_get (record, "location", &location, NULL);
		g_assert (NULL != location);
		g_warning ("Error opening %s", location);
		g_free (location);
		goto _return;
	}

	FILE *outfile = fopen (cachepath, "w");
	if (outfile == NULL) {
		 g_warning ("Error opening: %s", cachepath);
		 goto _return;
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
	if (NULL != outfile) {
		fclose (outfile);
	}

	if (NULL != decoded_stream) {
		g_input_stream_close (decoded_stream, NULL, NULL);
	}

	if (NULL != stream) {
		g_input_stream_close (stream, NULL, NULL); /* FIXME: should this be done in GGstMp3InputStream class? */
	}

	return;
}

/* NOTE: This is here and not in the individual DMAPRecords because records
 * have no knowledge of the database, db_dir, etc.
 */
void
transcode_cache (gpointer id, DAAPRecord *record, db_dir_and_target_transcode_mimetype_t *df)
{
	struct stat statbuf;
	gboolean has_video = FALSE;
	gchar *location = NULL;
	gchar *format = NULL;
	gchar *cacheuri = NULL;
	gchar *cachepath = NULL;
	guint64 filesize;

	g_assert (df->db_dir);
	g_assert (df->target_transcode_mimetype);

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
		goto _return;
	}

	gchar *format2 = dmap_mime_to_format (df->target_transcode_mimetype);
	if (NULL == format2) {
		g_warning ("Cannot transcode %s\n", df->target_transcode_mimetype);
		goto _return;
	}

	if (! strcmp (format, format2)) {
		g_debug ("Transcoding not necessary %s", location);
		goto _return;
	}

	cachepath = cache_path (CACHE_TYPE_TRANSCODED_DATA, df->db_dir, location);
	if (NULL == cachepath) {
		g_warning ("Could not determine cache path.");
		goto _return;
	}

	if (! g_file_test (cachepath, G_FILE_TEST_EXISTS)) {
		/* FIXME: return value, not void: */
		g_debug ("Transcoding %s to %s", location, cachepath);
		do_transcode (record, cachepath, df->target_transcode_mimetype);
	} else {
		g_debug ("Found transcoded data at %s for %s", cachepath, location);
	}

	if (-1 == stat (cachepath, &statbuf)) {
		g_warning ("Could not determine size of transcoded file.");
		goto _return;
	}
	filesize = statbuf.st_size;

	/* Replace previous location with URI to transcoded file. */
	cacheuri = g_filename_to_uri(cachepath, NULL, NULL);
	if (NULL == cacheuri) {
		g_warning ("Could not convert %s to URI.\n", cachepath);
		goto _return;
	}

	g_object_set (record, "location", cacheuri,
	                      "format",   format2,
	                      "filesize", filesize,
	                       NULL);

_return:
	if (location) {
		g_free (location);
	}
	if (format) {
		g_free (format);
	}
	if (cacheuri) {
		g_free (cacheuri);
	}
	if (cachepath) {
		g_free (cachepath);
	}
	if (format2) {
		g_free (format2);
	}

	return;
}
