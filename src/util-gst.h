/*   FILE: util-gst.h -- GStreamer utility functions
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

#ifndef __DMAPD_UTIL_GST
#define __DMAPD_UTIL_GST

#include <glib.h>
#include <gst/gst.h>

#include "dmapd-daap-record.h"

typedef struct {
	gchar *db_dir;
	gchar *target_transcode_mimetype;
} db_dir_and_target_transcode_mimetype_t;

// FIXME: split into two different impl.: GstElement *setup_pipeline (const char *sinkname);
gboolean pads_compatible (GstPad *pad1, GstPad *pad2);
gboolean transition_pipeline (GstElement *pipeline, GstState state);
void     transcode_cache (gpointer id, DAAPRecord *record, db_dir_and_target_transcode_mimetype_t* df);

#endif
