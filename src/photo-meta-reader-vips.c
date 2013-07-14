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
calculate_shrink (PhotoMetaReader *reader, int width, int height, double *residual)
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

/* Find the best jpeg preload shrink.
 */
static int
thumbnail_find_jpegshrink (PhotoMetaReader *reader, VipsImage *im)
{
	int shrink = calculate_shrink (reader, im->Xsize, im->Ysize, NULL);

	if (shrink >= 8)
		return 8;
	else if (shrink >= 4)
		return 4;
	else if (shrink >= 2)
		return 2;
	else 
		return 1;
}

#define THUMBNAIL "jpeg-thumbnail-data"

/* Try to read an embedded thumbnail. 
 */
static VipsImage *
thumbnail_get_thumbnail (PhotoMetaReader *reader, VipsImage *im)
{
	void *ptr;
	size_t size;
	VipsImage *thumb;
	double residual;
	int jpegshrink;

	if (!vips_image_get_typeof (im, THUMBNAIL) ||
		vips_image_get_blob (im, THUMBNAIL, &ptr, &size) ||
		vips_jpegload_buffer (ptr, size, &thumb, NULL)) {
		g_debug ("no jpeg thumbnail"); 
		return NULL; 
	}

	(void) calculate_shrink (reader, thumb->Xsize, thumb->Ysize, &residual);
	if (residual > 1.0) { 
		g_debug ("jpeg thumbnail too small"); 
		g_object_unref (thumb); 
		return NULL; 
	}

	/* Reload with the correct downshrink.
	 */
	jpegshrink = thumbnail_find_jpegshrink (reader, thumb);
	g_debug ("loading jpeg thumbnail with factor %d pre-shrink", jpegshrink);
	g_object_unref (thumb);
	if( vips_jpegload_buffer (ptr, size, &thumb, 
		"shrink", jpegshrink,
		NULL)) {
		g_debug ("jpeg thumbnail reload failed"); 
		return NULL; 
	}

	return thumb;
}

/* Open an image, returning the best version of that image for thumbnailing. 
 *
 * jpegs can have embedded thumbnails ... use that if it's large enough.
 *
 * libjpeg supports fast shrink-on-read, so if we have a JPEG, we can ask 
 * VIPS to load a lower resolution version.
 */
static VipsImage *
thumbnail_open (PhotoMetaReader *reader, VipsObject *process, const char *filename)
{
	const char *loader;
	VipsImage *im;
	int multiscan;

	g_debug ("thumbnailing %s", filename);

	if (!(loader = vips_foreign_find_load (filename)))
		return NULL;

	g_debug ("selected loader is \"%s\"", loader); 

	if (strcmp (loader, "VipsForeignLoadJpegFile") == 0) {
		VipsImage *thumb;

		/* This will just read in the header and is quick.
		 */
		if (!(im = vips_image_new_from_file (filename)))
			return NULL;
		vips_object_local (process, im);

		/* Try to read an embedded thumbnail. If we find one, use that
		 * instead.
		 */
		if ((thumb = thumbnail_get_thumbnail (reader, im))) { 
			vips_object_local (process, thumb);

			g_debug ("using %dx%d embedded jpeg thumbnail", thumb->Xsize, thumb->Ysize); 

			/* @thumb has not been fully decoded yet ... 
			 * we must not close @im until we're done with @thumb.
			 */
			vips_object_local (VIPS_OBJECT (thumb), im);

			im = thumb;
		} else if (vips_image_get_typeof (im, "jpeg-multiscan") &&
			!vips_image_get_int (im, "jpeg-multiscan", &multiscan) &&
			multiscan) {
			// libjpeg handles multiscan JPEGs differently.
			// Avoid this because of memory use on small devices.
			g_warning ("Will not try to thumbnail multiscan JPEG at %s", im->filename);
			return NULL;
		} else {
			int jpegshrink;

			g_debug ("processing main jpeg image");

			jpegshrink = thumbnail_find_jpegshrink (reader, im);

			g_debug ("loading jpeg with factor %d pre-shrink", jpegshrink); 

			if (vips_foreign_load (filename, &im,
				"sequential", TRUE,
				"shrink", jpegshrink,
				NULL))
				return NULL;
			vips_object_local (process, im);
		}
	}
	else {
		/* All other formats.
		 */
		if (vips_foreign_load (filename, &im,
			"sequential", TRUE,
			NULL))
			return NULL;
		vips_object_local (process, im);
	}

	return im; 
}

static gboolean
thumbnail_shrink (PhotoMetaReader *reader, VipsObject *process, VipsImage *in,
           void **thumb, size_t *size)
{
	VipsImage **t = (VipsImage **) vips_object_local_array (process, 10);

	int shrink; 
	double residual; 
	VipsInterpolate *interp;
	int tile_width;
	int tile_height;
	int nlines;

	/* Unpack the two coded formats we support.
	 */
	if (in->Coding == VIPS_CODING_LABQ) {
		g_debug ("unpacking LAB to RGB");

		if (vips_colourspace( in, &t[0], 
			VIPS_INTERPRETATION_sRGB, NULL)) 
			return FALSE; 

		in = t[0];
	}
	else if (in->Coding == IM_CODING_RAD) {
		g_debug ("unpacking Rad to float");

		/* rad is scrgb.
		 */
		if (vips_rad2float (in, &t[1], NULL) ||
			vips_colourspace (t[1], &t[2], VIPS_INTERPRETATION_sRGB, NULL)) 
			return FALSE;

		in = t[2];
	}

	shrink = calculate_shrink (reader, in->Xsize, in->Ysize, &residual);

	g_debug ("integer shrink by %d", shrink);

	if (vips_shrink (in, &t[3], shrink, shrink, NULL)) 
		return FALSE;
	in = t[3];

	/* For images smaller than the thumbnail, we upscale with nearest
	 * neighbor. Otherwise we make thumbnails that look fuzzy and awful.
	 */
	if (residual > 1.0) 
		interp = vips_interpolate_nearest_static ();
	else 
		interp = vips_interpolate_bilinear_static ();

	g_debug ("residual scale by %g", residual);
	g_debug ("%s interpolation", VIPS_OBJECT_GET_CLASS( interp )->nickname);

	vips_get_tile_size (in, &tile_width, &tile_height, &nlines);
	if (vips_tilecache (in, &t[4], 
		"tile_width", in->Xsize,
		"tile_height", 10,
		"max_tiles", (nlines * 2) / 10,
		"strategy", VIPS_CACHE_SEQUENTIAL,
		NULL) ||
		vips_affine (t[4], &t[5], residual, 0, 0, residual, NULL, 
			"interpolate", interp,
			NULL))  
		return FALSE;
	in = t[5];

	/* Generate thumbnail to memory buffer.
	 */
	if (vips_jpegsave_buffer (in, thumb, size, NULL))
		return FALSE;

	return TRUE;
}

static const char *
photo_meta_reader_vips_get_str (VipsImage *im, const char *field)
{
	char *str;

	if (!vips_image_get_typeof (im, field)) 
		return NULL;

	if (vips_image_get_string (im, field, &str)) {
		g_warning ("Failed to read %s from %s: %s", field, im->filename, vips_error_buffer ());
		vips_error_clear ();
		return NULL;
	}

	return str; 
}

static gboolean
photo_meta_reader_vips_read (PhotoMetaReader *reader, DPAPRecord *record, const gchar *path)
{
	VipsObject *process;
	VipsImage *im = NULL;
	const gchar *str;
	VipsImage *thumb = NULL;
	gchar *basename      = NULL;
	gchar *uri           = NULL;
	gchar *aspect_ratio  = NULL;
	void *thumbnail_data = NULL;
	GByteArray *thumbnail_array = NULL;
	gboolean fnval = FALSE;
	struct stat buf;
	gsize thumbnail_size = 0;

	/* Allocate all vips objects locally to this ... unref this to unref 
	 * everything we created during this operation.
	 */
	process = (VipsObject *) vips_image_new (); 

	g_debug ("Processing %s...", path);

	im = vips_image_new_from_file (path);
	if (NULL == im) {
		g_warning ("Could not open %s", path);
		goto _done;
	}
	vips_object_local (process, im);

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

	g_object_set (record, "format", vips_foreign_find_load (path), NULL);
	g_object_set (record, "pixel-height", im->Ysize, NULL);
	g_object_set (record, "pixel-width", im->Xsize, NULL);
	g_object_set (record, "comments", "", NULL);
	g_object_set (record, "creation-date", buf.st_mtime, NULL);
	g_object_set (record, "rating", 5, NULL); /* FIXME: also read from meta-data: */

	g_debug ("    Tag pixel width is %d.",  im->Xsize);
	g_debug ("    Tag pixel height is %d.", im->Ysize);

	aspect_ratio = g_strdup_printf ("%f", im->Xsize / (float) im->Ysize);
	if (NULL == aspect_ratio) {
		g_warning ("Could not set aspect ratio\n");
	} else {
		g_debug ("    Tag aspect ratio is %s.", aspect_ratio);
		g_object_set (record, "aspect-ratio", aspect_ratio, NULL);
	}

	if ((str = photo_meta_reader_vips_get_str (im, "exif-User Comment"))) {
		g_debug ("    Tag comments is %s.", str);
		g_object_set (record, "comments", str, NULL);
	}

	if ((str = photo_meta_reader_vips_get_str (im, "exif-Date and Time"))) {
		// Format is: "2007:10:05 00:20:26 (ASCII, 20 bytes)".
		if (strlen (str) < 19) {
			g_warning ("Bad timestamp string in %s: %s", im->filename, str);
		} else {
			char buf[10];

			vips_strncpy (buf, str, 10);
			buf[4] = 0x00;	// Cut off year.
			errno = 0;
			long l = strtol (buf, NULL, 10);
			if (errno) {
				g_warning ("Bad timestamp string in %s: %s", im->filename, buf);
			} else {
				// FIXME: Handle other than year!
				long timestamp = (l - 1970) * 365 * 24 * 60 * 60;
				g_debug ("    Tag creation date is %s.", buf);
				g_object_set (record, "creation-date", timestamp, NULL);
			}
		}
	}

	thumb = thumbnail_open (reader, process, path);
	if (NULL == thumb) {
		g_warning ("Could not open thumbnail for %s", path);
		goto _done;
	}
	if (thumbnail_shrink (reader, process, thumb, &thumbnail_data, &thumbnail_size)) {
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
	if (NULL != process) {
		g_object_unref (process);
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
