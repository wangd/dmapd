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

#include "util-gst.h"

GstElement *
setup_pipeline (const char *sinkname)
{
	GstElement *pipeline, *src, *decoder, *sink;

	/* Set up pipeline. */
	pipeline = gst_pipeline_new ("pipeline");

	src = gst_element_factory_make ("filesrc", "src");
	decoder = gst_element_factory_make ("decodebin", "decoder");
	sink = gst_element_factory_make (sinkname, "sink");

	if (pipeline == NULL || src == NULL || decoder == NULL || sink == NULL) {
		g_warning ("Error creating a GStreamer pipeline");
		goto _error;
	}

	gst_bin_add_many (GST_BIN (pipeline),
			  src,
			  decoder,
			  sink,
			  NULL);

	if (gst_element_link (src, decoder) == FALSE) {
		g_warning ("Error linking GStreamer pipeline");
		goto _error;
	}

	g_debug ("Pipeline complete");
	return pipeline;

_error:
	if (src != NULL)
		g_object_unref (src);
	if (decoder != NULL)
		g_object_unref (decoder);
	if (sink != NULL)
		g_object_unref (sink);

	return NULL;
}
