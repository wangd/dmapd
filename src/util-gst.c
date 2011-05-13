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

/* FIXME: copied from libdmapsharing: */
gboolean
pads_compatible (GstPad *pad1, GstPad *pad2)
{
        gboolean fnval;
        GstCaps *res, *caps1, *caps2;

        caps1 = gst_pad_get_caps (pad1);
        caps2 = gst_pad_get_caps (pad2);
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
