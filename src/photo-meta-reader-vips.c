/*
 * Photo metadata functions.
 *
 * Copyright (C) 2009 W. Michael Petullo <mike@flyn.org>
 *
 * Portions Copyright (C) 2010 John Cupitt, from VIPS LGPL code.
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

#include <config.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <vips/vips.h>

#include "photo-meta-reader-vips.h"
#include "dmapd-dpap-record.h"

const int DEFAULT_MAX_THUMBNAIL_WIDTH = 128;

static GOptionGroup *
photo_meta_reader_vips_get_option_group (PhotoMetaReader * reader)
{
	g_assert (IS_PHOTO_META_READER (reader));

	if (0 != vips_init ("dmapd")) {
		g_error ("Could not initialize VIPS\n");
	}

	/* VIPS caches operations, but this ends up in a lot of open file descriptors.
	 * This might change, but for now let's reduce the cache size.
	 * See vips-devel mailing list, Subj: im_open with "rd" mode, 14 July 2012.
	 */
	vips_cache_set_max(0);

	return vips_get_option_group ();
}

/* Calculate the shrink factors. 
 *
 * We shrink in two stages: first, a shrink with a block average. This can
 * only accurately shrink by integer factors. We then do a second shrink with
 * bilinear interpolation to get the exact size we want.
 */
static int
calculate_shrink (PhotoMetaReader * reader, int width, int height, double *residual)
{
	g_assert (NULL != reader);

	guint max_thumbnail_width = 0;

	g_object_get (reader, "max-thumbnail-width", &max_thumbnail_width, NULL);

	if (0 == max_thumbnail_width) {
		max_thumbnail_width = DEFAULT_MAX_THUMBNAIL_WIDTH;
	}

	g_debug ("    Maximum thumbnail width is %d.", max_thumbnail_width);

	/* We shrink to make the largest dimension equal to size. */
	int dimension = VIPS_MAX (width, height);

	double factor = dimension / (double) max_thumbnail_width;

	/* If the shrink factor is <= 1.0, we need to zoom rather than shrink.
	 * Just set the factor to 1 in this case.
	 */
	factor = factor < 1.0 ? 1.0 : factor;

	/* Int component of shrink. */
	int shrink = floor (factor);

	/* Size after int shrink. */
	int isize = floor (dimension / shrink);

	/* Therefore residual scale factor is. */
	if (residual) {
		*residual = max_thumbnail_width / (double) isize;
	}

	return shrink;
}

/* We use bilinear interpolation for the final shrink. VIPS has higher-order
 * interpolators, but they are only built if a C++ compiler is available.
 * Bilinear can look a little 'soft', so after shrinking, we need to sharpen a
 * little.
 *
 * This is a simple sharpen filter.
 */
static INTMASK *
sharpen_filter (void)
{
	static INTMASK *mask = NULL;

	if (NULL == mask) {
		mask = im_create_imaskv ("sharpen.con", 3, 3, -1, -1, -1, -1, 16, -1, -1, -1, -1);

		if (NULL == mask) {
			g_error ("Could not create VIPS mask");
		}

		mask->scale = 8;
	}

	return (mask);
}

static gboolean
thumbnail3 (PhotoMetaReader * reader, VipsImage * in, VipsImage * out)
{
	g_assert (IS_PHOTO_META_READER (reader));
	g_assert (NULL != in);
	g_assert (NULL != out);

	gboolean fnval = FALSE; 
	VipsImage *t[4] = { NULL, NULL, NULL, NULL };
	VipsImage *x = NULL;
	VipsInterpolate *interp = NULL;
	int shrink;
	double residual;

	shrink = calculate_shrink (reader, in->Xsize, in->Ysize, &residual);

	/* For images smaller than the thumbnail, we upscale with nearest
	 * neighbor. Otherwise we make thumbnails that look fuzzy and awful.
	 */
	if (residual > 1.0) {
		interp = vips_interpolate_nearest_static ();
	} else {
		interp = vips_interpolate_bilinear_static ();
	}

	if (NULL == interp) {
		g_warning ("Could not find interpolater");
		goto _done;
	}

	if (im_open_local_array (out, t, 4, "thumbnail", "p")) {
		g_warning ("Could not open local array");
		goto _done;
	}
	x = in;
	/* Unpack the two coded formats we support to float for processing.  */
	switch (x->Coding) {
		case IM_CODING_LABQ:
			g_debug ("    In im_LabQ2disp.");
			if (im_LabQ2disp (x, t[0], im_col_displays (7))) {
				g_warning ("    Failed.");
				goto _done;
			}
			g_debug ("    Done.");
			x = t[0];
			break;
		case IM_CODING_RAD:
			g_debug ("    In im_rad2float.");
			if (im_rad2float (x, t[0])) {
				g_warning ("    Failed.");
				goto _done;
			}
			g_debug ("    Done.");
			x = t[0];
			break;
		case IM_CODING_NONE:
			g_debug ("    No coding.");
			break;
		default:
			g_warning ("    Unknown coding.");
			goto _done;
	}

	g_debug ("    Shrinking.");
	if (vips_shrink (x, &t[1], shrink, shrink, NULL) ||
	    vips_affine (t[1], &t[2], residual, 0, 0, residual, "interpolate", interp)) {
		g_warning ("Failed");
		goto _done;
	}
	g_debug ("    Shrinking done.");
	x = t[2];

	/* If we are upsampling, don't sharpen, since nearest looks dumb sharpened. */
	if (residual > 1.0) {
		g_debug ("    Sharpening.");
		if (im_conv (x, t[3], sharpen_filter ())) {
			g_warning ("Failed");
			goto _done;
		}
		g_debug ("    Sharpening done.");
		x = t[3];
	}

	if (vips_image_write (x, out)) {
		g_warning ("Could not copy: %s", vips_error_buffer ());
		vips_error_clear ();
		goto _done;
	}

	fnval = TRUE;

_done:
	if (NULL != t[0]) {
		g_object_unref (t[0]);
	}

	if (NULL != t[1]) {
		g_object_unref (t[1]);
	}

	if (NULL != t[2]) {
		g_object_unref (t[2]);
	}

	if (NULL != t[3]) {
		g_object_unref (t[3]);
	}

	return fnval;
}

static gboolean
thumbnail2 (PhotoMetaReader * reader, VipsImage * in, VipsFormatClass * format,
           void **thumb, size_t * size)
{
	g_assert (IS_PHOTO_META_READER (reader));
	g_assert (NULL != in);
	g_assert (NULL != format);
	g_assert (NULL != thumb);
	g_assert (NULL != size);

	gint fd = -1;
	GError *error = NULL;
	VipsImage *out = NULL;
	gchar *thumbpath = NULL;
	gboolean got_thumb = FALSE;

	if ((fd = g_file_open_tmp ("photo-meta-reader-vips-XXXXXX.jpg", &thumbpath, &error)) == -1) {
		g_warning ("Unable to open temporary file for thumbnail: %s", error->message);
		g_error_free (error);
		goto _done;
	}

	if (!(out = vips_image_new_mode (thumbpath, "w"))) {
		goto _done;
	}

	if (FALSE == thumbnail3 (reader, in, out)) {
		goto _done;
	}

	if (!g_file_get_contents (thumbpath, (gchar **) thumb, size, &error)) {
		g_warning ("Error reading generated thumbnail at %s: %s",
		            thumbpath,
		            error->message);
		g_error_free (error);
	} else {
		g_debug ("    Generated thumbnail.");
		got_thumb = TRUE;
	}

_done:
	if (fd >= 0) {
		close (fd);
	}

	if (NULL != out) {
		g_object_unref (out);
	}

	if (NULL != thumbpath) {
		g_unlink (thumbpath);
		g_free (thumbpath);
	}

	return got_thumb;
}

static int
thumbnail (PhotoMetaReader *reader, VipsImage *in, VipsFormatClass *format,
	   void **thumb, size_t *size)
{
	g_assert (IS_PHOTO_META_READER (reader));
	g_assert (NULL != in);
	g_assert (NULL != format);
	g_assert (NULL != thumb);
	g_assert (NULL != size);

	gboolean got_thumb = FALSE;
	VipsImage *in2 = NULL;
	*size = 0;
	*thumb = NULL;

	if (strcmp (VIPS_OBJECT_CLASS (format)->nickname, "jpeg") == 0) {
		/* JPEGs get special treatment. */
		if (vips_image_get_typeof (in, "jpeg-thumbnail-data")) {
			void *ptr;

			if (0 != vips_image_get_blob (in, "jpeg-thumbnail-data", &ptr, size)) {
				g_warning ("Failed to read EXIF thumbnail %s: %s", in->filename, vips_error_buffer ());
				vips_error_clear ();
				goto _done;
			}

			g_debug ("    Read EXIF thumbnail of size %lu.", *size);

			// Process EXIF thumbnail instead (may need to shrink more).
			// This is expected to be small; open in memory.
			in2 = vips_image_new_mode ("thumb", "t");
			if (NULL == in2) {
				g_warning ("Could not open existing thumbnail: %s", vips_error_buffer ());
				vips_error_clear ();
				goto _done;
			}

			if (im_bufjpeg2vips (ptr, *size, in2, FALSE)) {
				g_warning ("Could not decode existing thumbnail: %s", vips_error_buffer ());
				vips_error_clear ();
				goto _done;
			}

			got_thumb = thumbnail2 (reader, in2, format, thumb, size);
			goto _done;

		} else if (vips_image_get_typeof (in, "jpeg-multiscan")) {
			// libjpeg handles multiscan JPEGs differently.
			// Avoid this because of memory use on small devices.

			int multiscan;

			if (vips_image_get_int (in, "jpeg-multiscan", &multiscan)) {
				g_warning ("Failed to determine if %s multiscan: %s",
				            in->filename, vips_error_buffer ());
				vips_error_clear ();
				goto _done;
			}

			if (multiscan) {
				g_warning ("Will not try to thumbnail multiscan JPEG at %s", in->filename);
				goto _done;
			}
		}
	}

	got_thumb = thumbnail2 (reader, in, format, thumb, size);

_done:
	if (NULL != in2) {
		g_object_unref (in2);
	}

	return got_thumb;
}

static gboolean
photo_meta_reader_vips_read (PhotoMetaReader *reader, DPAPRecord *record, const gchar *path)
{
	int x, y;
	VipsImage *im = NULL;
	gchar *str;
	gchar *basename      = NULL;
	gchar *uri           = NULL;
	gchar *aspect_ratio  = NULL;
	void *thumbnail_data = NULL;
	GByteArray *thumbnail_array = NULL;
	gboolean fnval = FALSE;
	VipsFormatClass *format;
	struct stat buf;
	gsize thumbnail_size = 0;

	g_debug ("Processing %s...", path);

	format = vips_format_for_file (path);
	if (NULL == format) {
		g_warning ("Do not know how to handle %s", path);
		goto _done;
	}

	// Might be big, decompress using disk.
	im = vips_image_new_mode (path, "rd");
	if (NULL == im) {
		g_warning ("Could not open %s", path);
		goto _done;
	}

	/* Get this here because it will be changed by thumbnailing process: */
	x = im->Xsize;
	y = im->Ysize;

	if (0 != stat (path, &buf)) {
		g_warning ("Unable to determine size of %s", path);
	} else {
		g_debug ("    Tag large filesize is %ld.", (unsigned long) buf.st_size);
		g_object_set (record, "large-filesize", buf.st_size, NULL);
	}

	basename = g_path_get_basename (path);
	if (NULL == basename) {
		g_warning ("Unable to determine basename of %s\n", path);
	} else {
		g_debug ("    Tag filename is %s.", basename);
		g_object_set (record, "filename", basename, NULL);
	}

	uri = g_filename_to_uri (path, NULL, NULL);
	if (NULL == uri) {
		g_warning ("Unable to determine URI for %s\n", path);
	} else {
		g_debug ("    Tag location is %s.", uri);
		g_object_set (record, "location", uri, NULL);
	}

	g_object_set (record, "format", VIPS_OBJECT_CLASS (format)->nickname, NULL);
	g_object_set (record, "pixel-height", im->Ysize, NULL);
	g_object_set (record, "pixel-width", im->Xsize, NULL);
	g_object_set (record, "comments", "", NULL);
	g_object_set (record, "creation-date", buf.st_mtime, NULL);
	g_object_set (record, "rating", 5, NULL); /* FIXME: also read from meta-data: */

	g_debug ("    Tag pixel width is %d.",  im->Xsize);
	g_debug ("    Tag pixel height is %d.", im->Ysize);

	aspect_ratio = g_strdup_printf ("%f", x / (float) y);
	if (NULL == aspect_ratio) {
		g_warning ("Could not set aspect ratio\n");
	} else {
		g_debug ("    Tag aspect ratio is %s.", aspect_ratio);
		g_object_set (record, "aspect-ratio", aspect_ratio, NULL);
	}

	if (vips_image_get_typeof (im, "exif-User Comment")) {
		if (vips_image_get_string (im, "exif-User Comment", &str)) {
			g_warning ("Failed to read comments from %s: %s", im->filename, vips_error_buffer ());
			vips_error_clear ();
		} else {
			g_debug ("    Tag comments is %s.", str);
			g_object_set (record, "comments", str, NULL);
		}
	}

	if (vips_image_get_typeof (im, "exif-Date and Time")) {
		if (vips_image_get_string (im, "exif-Date and Time", &str)) {
			g_warning ("Failed to read timestamp from %s: %s", im->filename, vips_error_buffer ());
			vips_error_clear ();
		} else {
			// Format is: "2007:10:05 00:20:26 (ASCII, 20 bytes)".
			if (strlen (str) < 19) {
				g_warning ("Bad timestamp string in %s: %s", im->filename, str);
			} else {
				str[4] = 0x00;	// Cut off year.
				errno = 0;
				long l = strtol (str, NULL, 10);
				if (errno) {
					g_warning ("Bad timestamp string in %s: %s", im->filename, str);
				} else {
					// FIXME: Handle other than year!
					long timestamp = (l - 1970) * 365 * 24 * 60 * 60;
					g_debug ("    Tag creation date is %s.", str);
					g_object_set (record, "creation-date", timestamp, NULL);
				}
			}
		}
	}

	if (thumbnail (reader, im, format, &thumbnail_data, &thumbnail_size)) {
		g_debug ("    Thumbnail is %ld bytes", thumbnail_size);
		thumbnail_array = g_byte_array_sized_new (thumbnail_size);
		if (NULL == thumbnail_array) {
			// FIXME: I think the thumbnail prop. MUST be non-NULL;
			//        if not, this could be cleaned up and non-fatal.
			g_error ("Could not allocate memory for thumbnail array\n");
		} else {
			g_byte_array_append (thumbnail_array, thumbnail_data, thumbnail_size);
		}
	} else {
		thumbnail_array = g_byte_array_sized_new (0);
		if (NULL == thumbnail_array) {
			// FIXME: I think the thumbnail prop. MUST be non-NULL;
			//        if not, this could be cleaned up and non-fatal.
			g_error ("Could not allocate memory for empty thumbnail array\n");
		}
	}
	g_object_set (record, "thumbnail", thumbnail_array, NULL);

	fnval = TRUE;

_done:
	if (NULL != im) {
		g_object_unref (im);
	}

	if (NULL != basename) {
		g_free (basename);
	}	

	if (NULL != uri) {
		g_free (uri);
	}	

	if (NULL != aspect_ratio) {
		g_free (aspect_ratio);
	}	

	if (NULL != thumbnail_data) {
		g_free (thumbnail_data);
	}

	if (NULL != thumbnail_array) {
		g_byte_array_unref (thumbnail_array);
	}

	return fnval;
}

static void
photo_meta_reader_vips_class_finalize (PhotoMetaReaderVipsClass * klass)
{
}

static void
photo_meta_reader_vips_init (PhotoMetaReaderVips * reader)
{
	/* reader->priv = PHOTO_META_READER_VIPS_GET_PRIVATE (reader); */
}

static void
photo_meta_reader_vips_class_init (PhotoMetaReaderVipsClass * klass)
{
	PhotoMetaReaderClass *photo_meta_reader =
		PHOTO_META_READER_CLASS (klass);

	/* g_type_class_add_private (klass, sizeof (PhotoMetaReaderVipsPrivate)); */

	photo_meta_reader->read = photo_meta_reader_vips_read;
	photo_meta_reader->get_option_group =
		photo_meta_reader_vips_get_option_group;
}

static void photo_meta_reader_vips_register_type (GTypeModule * module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule * module)
{
	photo_meta_reader_vips_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule * module)
{
	return TRUE;
}

G_DEFINE_DYNAMIC_TYPE (PhotoMetaReaderVips,
		       photo_meta_reader_vips, TYPE_PHOTO_META_READER)
