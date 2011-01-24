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

#include <math.h>
#include <string.h>
#include <vips/vips.h>

#include "photo-meta-reader-vips.h"
#include "dmapd-dpap-record.h"

const int THUMBNAIL_SIZE = 128;

/*
struct PhotoMetaReaderVipsPrivate {
};
*/

static void
photo_meta_reader_vips_set_property (GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
photo_meta_reader_vips_get_property (GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static GOptionGroup *
photo_meta_reader_vips_get_option_group (PhotoMetaReader *reader)
{
	im_init_world ("dmapd");
        return im_get_option_group ();
}


/* Calculate the shrink factors. 
 *
 * We shrink in two stages: first, a shrink with a block average. This can
 * only accurately shrink by integer factors. We then do a second shrink with
 * bilinear interpolation to get the exact size we want.
 */
static int
calculate_shrink( int width, int height, double *residual )
{
	/* We shrink to make the largest dimension equal to size.
	 */
	int dimension = IM_MAX( width, height );

	double factor = dimension / (double) THUMBNAIL_SIZE;

	/* If the shrink factor is <=1.0, we need to zoom rather than shrink.
	 * Just set the factor to 1 in this case.
	 */
	double factor2 = factor < 1.0 ? 1.0 : factor;

	/* Int component of shrink.
	 */
	int shrink = floor( factor2 );

	/* Size after int shrink.
	 */
	int isize = floor( dimension / shrink );

	/* Therefore residual scale factor is.
	 */
	if( residual )
		*residual = THUMBNAIL_SIZE / (double) isize;

	return( shrink );
}

/* We use bilinear interpolation for the final shrink. VIPS has higher-order
 * interpolators, but they are only built if a C++ compiler is available.
 * Bilinear can look a little 'soft', so after shrinking, we need to sharpen a
 * little.
 *
 * This is a simple sharpen filter.
 */
static INTMASK *
sharpen_filter( void )
{
	static INTMASK *mask = NULL;

	if( !mask ) {
		mask = im_create_imaskv( "sharpen.con", 3, 3, 
			-1, -1, -1, 
			-1, 16, -1, 
			-1, -1, -1 );
		mask->scale = 8;
	}

	return( mask );
}

static int
shrink_factor( IMAGE *in, IMAGE *out )
{
	IMAGE *t[9];
	IMAGE *x;
	int shrink;
	double residual;
	VipsInterpolate *interp;

	shrink = calculate_shrink( in->Xsize, in->Ysize, &residual );

	/* For images smaller than the thumbnail, we upscale with nearest
	 * neighbor. Otherwise we makes thumbnails that look fuzzy and awful.
	 */
	if( residual > 1.0 )
		interp = vips_interpolate_nearest_static();
	else
		/* FIXME: valgrind possible leak */
		interp = vips_interpolate_bilinear_static();

	if( im_open_local_array( out, t, 9, "thumbnail", "p" ) )
		return( -1 );
	x = in;

	/* Unpack the two coded formats we support to float for processing.
	 */
	if( x->Coding == IM_CODING_LABQ ) {
		g_debug ("in im_LabQ2disp");
		if( im_LabQ2disp( x, t[1], im_col_displays( 7 ) ) )
			return( -1 );
		g_debug ("Done");
		x = t[1];
	}
	else if( x->Coding == IM_CODING_RAD ) {
		g_debug ("in im_rad2float");
		if( im_rad2float( x, t[1] ) )
			return( -1 );
		g_debug ("Done");
		x = t[1];
	}

	/* Shrink!
	 */
	g_debug ("Shrinking");
	if( im_shrink( x, t[2], shrink, shrink ) ||
		im_affinei_all( t[2], t[3], 
			interp, residual, 0, 0, residual, 0, 0 ) )
		return( -1 );
	g_debug ("Shrinking done");
	x = t[3];

	/* If we are upsampling, don't sharpen, since nearest looks dumb
	 * sharpened.
	 */
	if( residual > 1.0 ) {
		g_debug ("Sharpening");
		if( im_conv( x, t[4], sharpen_filter() ) )
			return( -1 );
		g_debug ("Sharpening done");
		x = t[4];
	}

	/* FIXME: valgrind leak: */
	if( im_copy( x, out ) )
		return( -1 );

	return( 0 );
}

static int
thumbnail( IMAGE *in, VipsFormatClass *format, void **thumb, size_t *size)
{
	gint fd;
	IMAGE *out;
	int multiscan;
	gchar *thumbpath;
	GError *error = NULL;
	gboolean got_thumb = FALSE;

	if( strcmp( VIPS_OBJECT_CLASS( format )->nickname, "jpeg" ) == 0 ) {
		/* JPEGs get special treatment. libjpeg supports fast shrink-on-read,
		 * so if we have a JPEG, we can ask VIPS to load a lower resolution
		 * version.
		 */
		int shrink;
		char buf[FILENAME_MAX];

		if (im_header_get_typeof (in, "jpeg-thumbnail-data")) {
			void *ptr;
			if (im_meta_get_blob (in, "jpeg-thumbnail-data", &ptr, size)) {
				g_warning ("Failed to read EXIF thumbnail %s: %s", in->filename, im_error_buffer ());
				im_error_clear ();
			} else {
				*thumb = g_new (gchar, *size);

				/* FIXME: leak or correct? */
				memcpy (*thumb, ptr, *size);

				g_debug ("Read EXIF thumbnail of size %lu", *size);
				got_thumb = TRUE;
				goto _done_no_out;
			}
		}

		if (im_header_get_typeof (in, "jpeg-multiscan")) {
			if (im_header_int (in, "jpeg-multiscan", &multiscan)) {
				g_warning ("Failed to determine if %s multiscan: %s", in->filename, im_error_buffer ());
				im_error_clear ();
				goto _done_no_out;
			}
			if (multiscan) {
				g_warning ("Will not try to thumbnail multiscan JPEG at %s", in->filename);
				goto _done_no_out;
			}
		}

		shrink = calculate_shrink( in->Xsize, in->Ysize, NULL );

		if( shrink > 8 )
			shrink = 8;
		else if( shrink > 4 )
			shrink = 4;
		else if( shrink > 2 )
			shrink = 2;
		else 
			shrink = 1;

		im_snprintf( buf, FILENAME_MAX, "%s:%d", in->filename, shrink );
		g_debug ("Opening %s:%d", in->filename, shrink);
		im_close( in );
		if( !(in = im_open( buf, "rd" )) )
			goto _done_no_out;
	}

	if ((fd = g_file_open_tmp ("photo-meta-reader-vips-XXXXXX.jpg", &thumbpath, &error)) == -1) {
		g_error ("Unable to open temporary file for thumbnail: %s", error->message);
	}
	close (fd);
	if( !(out = im_open( thumbpath , "w" )) ) {
		im_close( in );
		g_free(thumbpath);
		goto _done_no_out;
	}

	if (shrink_factor( in, out ) || ! g_file_get_contents (thumbpath, (gchar **) thumb, size, &error)) {
		*size = 0;
		*thumb = NULL;
		g_warning ("Error reading generated thumbnail at %s", thumbpath);
	} else {
		g_debug ("Generated thumbnail");
		got_thumb = TRUE;
	}

	g_unlink(thumbpath);
	g_free(thumbpath);

_done:
	im_close( out );

_done_no_out:
	im_close( in );

	return got_thumb;
}

static gboolean
photo_meta_reader_vips_read (PhotoMetaReader *reader,
			     DPAPRecord *record,
			     const gchar *path)
{
	IMAGE *im;
	int x, y;
	gboolean fnval = FALSE;
	VipsFormatClass *format;
	GError *error = NULL;
	struct stat buf;
	gchar *aspect_ratio_str;
	gchar *location;
	gchar *comments;
	GByteArray *thumbnail_array = NULL;
	void *thumbnail_data = NULL;
	gsize  thumbnail_size = 0;

	g_debug ("Processing %s", path);

	/* FIXME: valgrind possible leak */
	if( !(format = vips_format_for_file( path)) ) {
		g_warning ("Do not know how to handle %s", path);
		goto _done_no_im;
	}

	if( !(im = im_open( path, "rd" )) ) {
		g_warning ("Could not open %s", path);
		goto _done_no_im;
	}

	/* Get this here because it will be changed by thumbnailing process: */
	x = im->Xsize;
	y = im->Ysize;

	fnval = TRUE;

	if (stat (path, &buf) == -1) {
		g_warning ("Unable to determine size of %s", path);
	} else {
		g_object_set (record, "large-filesize", buf.st_size, NULL);
	}

	g_object_set (record, "filename", g_basename (path), NULL);
	location = g_strdup_printf ("file://%s", path);
	g_object_set (record, "location", location, NULL);
	g_free (location);

	g_object_set (record, "format", VIPS_OBJECT_CLASS(format)->nickname, NULL);
	g_object_set (record, "pixel-height", im->Ysize, NULL);
	g_object_set (record, "pixel-width", im->Xsize, NULL);
	g_object_set (record, "comments", "", NULL);

	aspect_ratio_str = g_strdup_printf ("%f", x / (float) y);
	g_object_set (record, "aspect-ratio", aspect_ratio_str, NULL);
	g_free (aspect_ratio_str);

	if (im_header_get_typeof (im, "exif-User Comment")) {
		if (im_meta_get_string (im, "exif-User Comment", &comments)) {
			g_warning ("Failed to read comments from %s: %s", im->filename, im_error_buffer ());
			im_error_clear ();
		} else {
			/* FIXME: leak or correct? */
			g_object_set (record, "comments", g_strdup (comments), NULL);
		}
	}

	/* FIXME: Also, exif-Date and Time looks like "2007:10:05 00:20:26 (ASCII, 20 bytes)" */
	/* FIXME: also read from meta-data: */
	g_object_set (record, "creation-date", 1, NULL);
	g_object_set (record, "rating", 5, NULL);

	/* WARNING: this must be the last function that uses im, because thumbnail closes im: */
	if (thumbnail (im, format, &thumbnail_data, &thumbnail_size)) {
		thumbnail_array = g_byte_array_sized_new (thumbnail_size);
		g_byte_array_append (thumbnail_array, thumbnail_data, thumbnail_size);
	}
	g_object_set (record, "thumbnail", thumbnail_array, NULL);

	fnval = TRUE;

_done_no_im:
	return fnval;
}

static void
photo_meta_reader_vips_class_finalize (PhotoMetaReaderVipsClass *klass)
{
}

static void photo_meta_reader_vips_init (PhotoMetaReaderVips *reader)
{
	/* reader->priv = PHOTO_META_READER_VIPS_GET_PRIVATE (reader); */
}

static void photo_meta_reader_vips_class_init (PhotoMetaReaderVipsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	PhotoMetaReaderClass *photo_meta_reader = PHOTO_META_READER_CLASS (klass);

	/* g_type_class_add_private (klass, sizeof (PhotoMetaReaderVipsPrivate)); */

	gobject_class->set_property = photo_meta_reader_vips_set_property;
	gobject_class->get_property = photo_meta_reader_vips_get_property;

        photo_meta_reader->read = photo_meta_reader_vips_read;
	photo_meta_reader->get_option_group = photo_meta_reader_vips_get_option_group;
}

static void photo_meta_reader_vips_register_type (GTypeModule *module);

G_MODULE_EXPORT gboolean
dmapd_module_load (GTypeModule *module)
{
        photo_meta_reader_vips_register_type (module);
	return TRUE;
}

G_MODULE_EXPORT gboolean
dmapd_module_unload (GTypeModule *module)
{
	return TRUE;
}

G_DEFINE_DYNAMIC_TYPE (PhotoMetaReaderVips,
                       photo_meta_reader_vips,
                       TYPE_PHOTO_META_READER)
