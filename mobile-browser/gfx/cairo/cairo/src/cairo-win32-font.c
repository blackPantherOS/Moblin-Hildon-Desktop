/* -*- Mode: c; tab-width: 8; c-basic-offset: 4; indent-tabs-mode: t; -*- */
/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2005 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is Red Hat, Inc.
 *
 * Contributor(s):
 */

#include "cairoint.h"

#include "cairo-win32-private.h"

#ifndef SPI_GETFONTSMOOTHINGTYPE
#define SPI_GETFONTSMOOTHINGTYPE 0x200a
#endif
#ifndef FE_FONTSMOOTHINGCLEARTYPE
#define FE_FONTSMOOTHINGCLEARTYPE 2
#endif
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif
#ifndef TT_PRIM_CSPLINE
#define TT_PRIM_CSPLINE 3
#endif

#define OPENTYPE_CFF_TAG 0x20464643

const cairo_scaled_font_backend_t cairo_win32_scaled_font_backend;

typedef struct {
    cairo_scaled_font_t base;

    LOGFONTW logfont;

    BYTE quality;

    /* We do drawing and metrics computation in a "logical space" which
     * is similar to font space, except that it is scaled by a factor
     * of the (desired font size) * (WIN32_FONT_LOGICAL_SCALE). The multiplication
     * by WIN32_FONT_LOGICAL_SCALE allows for sub-pixel precision.
     */
    double logical_scale;

    /* The size we should actually request the font at from Windows; differs
     * from the logical_scale because it is quantized for orthogonal
     * transformations
     */
    double logical_size;

    /* Transformations from device <=> logical space
     */
    cairo_matrix_t logical_to_device;
    cairo_matrix_t device_to_logical;

    /* We special case combinations of 90-degree-rotations, scales and
     * flips ... that is transformations that take the axes to the
     * axes. If preserve_axes is true, then swap_axes/swap_x/swap_y
     * encode the 8 possibilities for orientation (4 rotation angles with
     * and without a flip), and scale_x, scale_y the scale components.
     */
    cairo_bool_t preserve_axes;
    cairo_bool_t swap_axes;
    cairo_bool_t swap_x;
    cairo_bool_t swap_y;
    double x_scale;
    double y_scale;

    /* The size of the design unit of the font
     */
    int em_square;

    HFONT scaled_hfont;
    HFONT unscaled_hfont;

    cairo_bool_t is_truetype;
    cairo_bool_t glyph_indexing;

    cairo_bool_t delete_scaled_hfont;
} cairo_win32_scaled_font_t;

static cairo_status_t
_cairo_win32_scaled_font_set_metrics (cairo_win32_scaled_font_t *scaled_font);

static cairo_status_t
_cairo_win32_scaled_font_init_glyph_metrics (cairo_win32_scaled_font_t *scaled_font,
					     cairo_scaled_glyph_t      *scaled_glyph);

static cairo_status_t
_cairo_win32_scaled_font_init_glyph_path (cairo_win32_scaled_font_t *scaled_font,
					  cairo_scaled_glyph_t      *scaled_glyph);

#define NEARLY_ZERO(d) (fabs(d) < (1. / 65536.))

static void
_compute_transform (cairo_win32_scaled_font_t *scaled_font,
		    cairo_matrix_t            *sc)
{
    cairo_status_t status;

    if (NEARLY_ZERO (sc->yx) && NEARLY_ZERO (sc->xy)) {
	scaled_font->preserve_axes = TRUE;
	scaled_font->x_scale = sc->xx;
	scaled_font->swap_x = (sc->xx < 0);
	scaled_font->y_scale = sc->yy;
	scaled_font->swap_y = (sc->yy < 0);
	scaled_font->swap_axes = FALSE;

    } else if (NEARLY_ZERO (sc->xx) && NEARLY_ZERO (sc->yy)) {
	scaled_font->preserve_axes = TRUE;
	scaled_font->x_scale = sc->yx;
	scaled_font->swap_x = (sc->yx < 0);
	scaled_font->y_scale = sc->xy;
	scaled_font->swap_y = (sc->xy < 0);
	scaled_font->swap_axes = TRUE;

    } else {
	scaled_font->preserve_axes = FALSE;
	scaled_font->swap_x = scaled_font->swap_y = scaled_font->swap_axes = FALSE;
    }

    if (scaled_font->preserve_axes) {
	if (scaled_font->swap_x)
	    scaled_font->x_scale = - scaled_font->x_scale;
	if (scaled_font->swap_y)
	    scaled_font->y_scale = - scaled_font->y_scale;

	scaled_font->logical_scale = WIN32_FONT_LOGICAL_SCALE * scaled_font->y_scale;
	scaled_font->logical_size = WIN32_FONT_LOGICAL_SCALE *
                                    _cairo_lround (scaled_font->y_scale);
    }

    /* The font matrix has x and y "scale" components which we extract and
     * use as character scale values.
     */
    cairo_matrix_init (&scaled_font->logical_to_device,
		       sc->xx, sc->yx, sc->xy, sc->yy, 0, 0);

    if (!scaled_font->preserve_axes) {
	_cairo_matrix_compute_scale_factors (&scaled_font->logical_to_device,
					     &scaled_font->x_scale, &scaled_font->y_scale,
					     TRUE);	/* XXX: Handle vertical text */

	scaled_font->logical_size = _cairo_lround (WIN32_FONT_LOGICAL_SCALE *
                                                   scaled_font->y_scale);
	scaled_font->logical_scale = WIN32_FONT_LOGICAL_SCALE * scaled_font->y_scale;
    }

    cairo_matrix_scale (&scaled_font->logical_to_device,
			1.0 / scaled_font->logical_scale, 1.0 / scaled_font->logical_scale);

    scaled_font->device_to_logical = scaled_font->logical_to_device;

    status = cairo_matrix_invert (&scaled_font->device_to_logical);
    if (status)
	cairo_matrix_init_identity (&scaled_font->device_to_logical);
}

static cairo_bool_t
_have_cleartype_quality (void)
{
    OSVERSIONINFO version_info;

    version_info.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

    if (!GetVersionEx (&version_info)) {
	_cairo_win32_print_gdi_error ("_have_cleartype_quality");
	return FALSE;
    }

    return (version_info.dwMajorVersion > 5 ||
	    (version_info.dwMajorVersion == 5 &&
	     version_info.dwMinorVersion >= 1));	/* XP or newer */
}

static BYTE
_get_system_quality (void)
{
    BOOL font_smoothing;
    UINT smoothing_type;

    if (!SystemParametersInfo (SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0)) {
	_cairo_win32_print_gdi_error ("_get_system_quality");
	return DEFAULT_QUALITY;
    }

    if (font_smoothing) {
	if (_have_cleartype_quality ()) {
	    if (!SystemParametersInfo (SPI_GETFONTSMOOTHINGTYPE,
				       0, &smoothing_type, 0)) {
		_cairo_win32_print_gdi_error ("_get_system_quality");
		return DEFAULT_QUALITY;
	    }

	    if (smoothing_type == FE_FONTSMOOTHINGCLEARTYPE)
		return CLEARTYPE_QUALITY;
	}

	return ANTIALIASED_QUALITY;
    } else {
	return DEFAULT_QUALITY;
    }
}

/* If face_hfont is non-NULL then font_matrix must be a simple scale by some
 * factor S, ctm must be the identity, logfont->lfHeight must be -S,
 * logfont->lfWidth, logfont->lfEscapement, logfont->lfOrientation must
 * all be 0, and face_hfont is the result of calling CreateFontIndirectW on
 * logfont.
 */
static cairo_status_t
_win32_scaled_font_create (LOGFONTW                   *logfont,
			   HFONT                      face_hfont,
			   cairo_font_face_t	      *font_face,
			   const cairo_matrix_t       *font_matrix,
			   const cairo_matrix_t       *ctm,
			   const cairo_font_options_t *options,
			   cairo_scaled_font_t       **font_out)
{
    cairo_win32_scaled_font_t *f;
    cairo_matrix_t scale;
    cairo_status_t status;

    f = malloc (sizeof(cairo_win32_scaled_font_t));
    if (f == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    f->logfont = *logfont;

    /* We don't have any control over the hinting style or subpixel
     * order in the Win32 font API, so we ignore those parts of
     * cairo_font_options_t. We use the 'antialias' field to set
     * the 'quality'.
     *
     * XXX: The other option we could pay attention to, but don't
     *      here is the hint_metrics options.
     */
    if (options->antialias == CAIRO_ANTIALIAS_DEFAULT)
	f->quality = _get_system_quality ();
    else {
	switch (options->antialias) {
	case CAIRO_ANTIALIAS_NONE:
	    f->quality = NONANTIALIASED_QUALITY;
	    break;
	case CAIRO_ANTIALIAS_GRAY:
	    f->quality = ANTIALIASED_QUALITY;
	    break;
	case CAIRO_ANTIALIAS_SUBPIXEL:
	    if (_have_cleartype_quality ())
		f->quality = CLEARTYPE_QUALITY;
	    else
		f->quality = ANTIALIASED_QUALITY;
	    break;
	case CAIRO_ANTIALIAS_DEFAULT:
	    ASSERT_NOT_REACHED;
	}
    }

    f->em_square = 0;
    f->scaled_hfont = NULL;
    f->unscaled_hfont = NULL;

    if (f->quality == logfont->lfQuality ||
        (logfont->lfQuality == DEFAULT_QUALITY &&
         options->antialias == CAIRO_ANTIALIAS_DEFAULT)) {
        /* If face_hfont is non-NULL, then we can use it to avoid creating our
         * own --- because the constraints on face_hfont mentioned above
         * guarantee it was created in exactly the same way that
         * _win32_scaled_font_get_scaled_hfont would create it.
         */
        f->scaled_hfont = face_hfont;
    }
    /* don't delete the hfont if we're using the one passed in to us */
    f->delete_scaled_hfont = !f->scaled_hfont;

    cairo_matrix_multiply (&scale, font_matrix, ctm);
    _compute_transform (f, &scale);

    status = _cairo_scaled_font_init (&f->base, font_face,
				      font_matrix, ctm, options,
				      &cairo_win32_scaled_font_backend);
    if (status)
	goto FAIL;

    status = _cairo_win32_scaled_font_set_metrics (f);
    if (status) {
	_cairo_scaled_font_fini (&f->base);
	goto FAIL;
    }

    *font_out = &f->base;
    return CAIRO_STATUS_SUCCESS;

 FAIL:
    free (f);
    return status;
}

static cairo_status_t
_win32_scaled_font_set_world_transform (cairo_win32_scaled_font_t *scaled_font,
					HDC                        hdc)
{
    XFORM xform;

    _cairo_matrix_to_win32_xform (&scaled_font->logical_to_device, &xform);

    if (!SetWorldTransform (hdc, &xform))
	return _cairo_win32_print_gdi_error ("_win32_scaled_font_set_world_transform");

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_win32_scaled_font_set_identity_transform (HDC hdc)
{
    if (!ModifyWorldTransform (hdc, NULL, MWT_IDENTITY))
	return _cairo_win32_print_gdi_error ("_win32_scaled_font_set_identity_transform");

    return CAIRO_STATUS_SUCCESS;
}

static HDC
_get_global_font_dc (void)
{
    static HDC hdc;

    if (!hdc) {
	hdc = CreateCompatibleDC (NULL);
	if (!hdc) {
	    _cairo_win32_print_gdi_error ("_get_global_font_dc");
	    return NULL;
	}

	if (!SetGraphicsMode (hdc, GM_ADVANCED)) {
	    _cairo_win32_print_gdi_error ("_get_global_font_dc");
	    DeleteDC (hdc);
	    return NULL;
	}
    }

    return hdc;
}

static HFONT
_win32_scaled_font_get_scaled_hfont (cairo_win32_scaled_font_t *scaled_font)
{
    if (!scaled_font->scaled_hfont) {
	LOGFONTW logfont = scaled_font->logfont;
	logfont.lfHeight = -scaled_font->logical_size;
	logfont.lfWidth = 0;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfQuality = scaled_font->quality;

	scaled_font->scaled_hfont = CreateFontIndirectW (&logfont);
	if (!scaled_font->scaled_hfont) {
	    _cairo_win32_print_gdi_error ("_win32_scaled_font_get_scaled_hfont");
	    return NULL;
	}
    }

    return scaled_font->scaled_hfont;
}

static HFONT
_win32_scaled_font_get_unscaled_hfont (cairo_win32_scaled_font_t *scaled_font,
				       HDC                        hdc)
{
    if (!scaled_font->unscaled_hfont) {
	OUTLINETEXTMETRIC *otm;
	unsigned int otm_size;
	HFONT scaled_hfont;
	LOGFONTW logfont;

	scaled_hfont = _win32_scaled_font_get_scaled_hfont (scaled_font);
	if (!scaled_hfont)
	    return NULL;

	if (!SelectObject (hdc, scaled_hfont)) {
	    _cairo_win32_print_gdi_error ("_win32_scaled_font_get_unscaled_hfont:SelectObject");
	    return NULL;
	}

	otm_size = GetOutlineTextMetrics (hdc, 0, NULL);
	if (!otm_size) {
	    _cairo_win32_print_gdi_error ("_win32_scaled_font_get_unscaled_hfont:GetOutlineTextMetrics");
	    return NULL;
	}

	otm = malloc (otm_size);
	if (!otm) {
	    _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	    return NULL;
	}

	if (!GetOutlineTextMetrics (hdc, otm_size, otm)) {
	    _cairo_win32_print_gdi_error ("_win32_scaled_font_get_unscaled_hfont:GetOutlineTextMetrics");
	    free (otm);
	    return NULL;
	}

	scaled_font->em_square = otm->otmEMSquare;
	free (otm);

	logfont = scaled_font->logfont;
	logfont.lfHeight = -scaled_font->em_square;
	logfont.lfWidth = 0;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfQuality = scaled_font->quality;

	scaled_font->unscaled_hfont = CreateFontIndirectW (&logfont);
	if (!scaled_font->unscaled_hfont) {
	    _cairo_win32_print_gdi_error ("_win32_scaled_font_get_unscaled_hfont:CreateIndirect");
	    return NULL;
	}
    }

    return scaled_font->unscaled_hfont;
}

static cairo_status_t
_cairo_win32_scaled_font_select_unscaled_font (cairo_scaled_font_t *scaled_font,
					       HDC                  hdc)
{
    cairo_status_t status;
    HFONT hfont;
    HFONT old_hfont = NULL;

    hfont = _win32_scaled_font_get_unscaled_hfont ((cairo_win32_scaled_font_t *)scaled_font, hdc);
    if (!hfont)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    old_hfont = SelectObject (hdc, hfont);
    if (!old_hfont)
	return _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_select_unscaled_font");

    status = _win32_scaled_font_set_identity_transform (hdc);
    if (status) {
	SelectObject (hdc, old_hfont);
	return status;
    }

    SetMapMode (hdc, MM_TEXT);

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_win32_scaled_font_done_unscaled_font (cairo_scaled_font_t *scaled_font)
{
}

/* implement the font backend interface */

static cairo_status_t
_cairo_win32_scaled_font_create_toy (cairo_toy_font_face_t *toy_face,
				     const cairo_matrix_t        *font_matrix,
				     const cairo_matrix_t        *ctm,
				     const cairo_font_options_t  *options,
				     cairo_scaled_font_t        **scaled_font_out)
{
    LOGFONTW logfont;
    uint16_t *face_name;
    int face_name_len;
    cairo_status_t status;

    status = _cairo_utf8_to_utf16 (toy_face->family, -1,
				   &face_name, &face_name_len);
    if (status)
	return status;

    if (face_name_len > LF_FACESIZE - 1) {
	free (face_name);
	return _cairo_error (CAIRO_STATUS_INVALID_STRING);
    }

    memcpy (logfont.lfFaceName, face_name, sizeof (uint16_t) * (face_name_len + 1));
    free (face_name);

    logfont.lfHeight = 0;	/* filled in later */
    logfont.lfWidth = 0;	/* filled in later */
    logfont.lfEscapement = 0;	/* filled in later */
    logfont.lfOrientation = 0;	/* filled in later */

    switch (toy_face->weight) {
    case CAIRO_FONT_WEIGHT_NORMAL:
    default:
	logfont.lfWeight = FW_NORMAL;
	break;
    case CAIRO_FONT_WEIGHT_BOLD:
	logfont.lfWeight = FW_BOLD;
	break;
    }

    switch (toy_face->slant) {
    case CAIRO_FONT_SLANT_NORMAL:
    default:
	logfont.lfItalic = FALSE;
	break;
    case CAIRO_FONT_SLANT_ITALIC:
    case CAIRO_FONT_SLANT_OBLIQUE:
	logfont.lfItalic = TRUE;
	break;
    }

    logfont.lfUnderline = FALSE;
    logfont.lfStrikeOut = FALSE;
    /* The docs for LOGFONT discourage using this, since the
     * interpretation is locale-specific, but it's not clear what
     * would be a better alternative.
     */
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = DEFAULT_QUALITY; /* filled in later */
    logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    if (!logfont.lfFaceName)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    return _win32_scaled_font_create (&logfont, NULL, &toy_face->base,
			              font_matrix, ctm, options,
				      scaled_font_out);
}

static void
_cairo_win32_scaled_font_fini (void *abstract_font)
{
    cairo_win32_scaled_font_t *scaled_font = abstract_font;

    if (scaled_font == NULL)
	return;

    if (scaled_font->scaled_hfont && scaled_font->delete_scaled_hfont)
	DeleteObject (scaled_font->scaled_hfont);

    if (scaled_font->unscaled_hfont)
	DeleteObject (scaled_font->unscaled_hfont);
}

static cairo_int_status_t
_cairo_win32_scaled_font_text_to_glyphs (void		*abstract_font,
					 double		x,
					 double		y,
					 const char	*utf8,
					 cairo_glyph_t **glyphs,
					 int		*num_glyphs)
{
    cairo_win32_scaled_font_t *scaled_font = abstract_font;
    uint16_t *utf16;
    int n16;
    GCP_RESULTSW gcp_results;
    unsigned int buffer_size, i;
    WCHAR *glyph_indices = NULL;
    int *dx = NULL;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    double x_pos, y_pos;
    double x_incr, y_incr;
    HDC hdc = NULL;

    /* Compute a vector in user space along the baseline of length one logical space unit */
    x_incr = 1;
    y_incr = 0;
    cairo_matrix_transform_distance (&scaled_font->base.font_matrix, &x_incr, &y_incr);
    x_incr /= scaled_font->logical_scale;
    y_incr /= scaled_font->logical_scale;

    status = _cairo_utf8_to_utf16 (utf8, -1, &utf16, &n16);
    if (status)
	return status;

    gcp_results.lStructSize = sizeof (GCP_RESULTS);
    gcp_results.lpOutString = NULL;
    gcp_results.lpOrder = NULL;
    gcp_results.lpCaretPos = NULL;
    gcp_results.lpClass = NULL;

    buffer_size = MAX (n16 * 1.2, 16);		/* Initially guess number of chars plus a few */
    if (buffer_size > INT_MAX) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FAIL1;
    }

    hdc = _get_global_font_dc ();
    if (!hdc) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FAIL1;
    }

    status = cairo_win32_scaled_font_select_font (&scaled_font->base, hdc);
    if (status)
	goto FAIL1;

    while (TRUE) {
	if (glyph_indices) {
	    free (glyph_indices);
	    glyph_indices = NULL;
	}
	if (dx) {
	    free (dx);
	    dx = NULL;
	}

	glyph_indices = _cairo_malloc_ab (buffer_size, sizeof (WCHAR));
	dx = _cairo_malloc_ab (buffer_size, sizeof (int));
	if (!glyph_indices || !dx) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto FAIL2;
	}

	gcp_results.nGlyphs = buffer_size;
	gcp_results.lpDx = dx;
	gcp_results.lpGlyphs = glyph_indices;

	if (!GetCharacterPlacementW (hdc, utf16, n16,
				     0,
				     &gcp_results,
				     GCP_DIACRITIC | GCP_LIGATE | GCP_GLYPHSHAPE | GCP_REORDER)) {
	    status = _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_text_to_glyphs");
	    goto FAIL2;
	}

	if (gcp_results.lpDx && gcp_results.lpGlyphs)
	    break;

	/* Too small a buffer, try again */

	buffer_size *= 1.5;
	if (buffer_size > INT_MAX) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto FAIL2;
	}
    }

    *num_glyphs = gcp_results.nGlyphs;
    *glyphs = _cairo_malloc_ab (gcp_results.nGlyphs, sizeof (cairo_glyph_t));
    if (!*glyphs) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FAIL2;
    }

    x_pos = x;
    y_pos = y;

    for (i = 0; i < gcp_results.nGlyphs; i++) {
	(*glyphs)[i].index = glyph_indices[i];
	(*glyphs)[i].x = x_pos ;
	(*glyphs)[i].y = y_pos;

	x_pos += x_incr * dx[i];
	y_pos += y_incr * dx[i];
    }

 FAIL2:
    if (glyph_indices)
	free (glyph_indices);
    if (dx)
	free (dx);

    cairo_win32_scaled_font_done_font (&scaled_font->base);

 FAIL1:
    free (utf16);

    return status;
}

static cairo_status_t
_cairo_win32_scaled_font_set_metrics (cairo_win32_scaled_font_t *scaled_font)
{
    cairo_status_t status;
    cairo_font_extents_t extents;

    TEXTMETRIC metrics;
    HDC hdc;

    hdc = _get_global_font_dc ();
    if (!hdc)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (scaled_font->preserve_axes || scaled_font->base.options.hint_metrics == CAIRO_HINT_METRICS_OFF) {
	/* For 90-degree rotations (including 0), we get the metrics
	 * from the GDI in logical space, then convert back to font space
	 */
	status = cairo_win32_scaled_font_select_font (&scaled_font->base, hdc);
	if (status)
	    return status;
	GetTextMetrics (hdc, &metrics);
	cairo_win32_scaled_font_done_font (&scaled_font->base);

	extents.ascent = metrics.tmAscent / scaled_font->logical_scale;
	extents.descent = metrics.tmDescent / scaled_font->logical_scale;

	extents.height = (metrics.tmHeight + metrics.tmExternalLeading) / scaled_font->logical_scale;
	extents.max_x_advance = metrics.tmMaxCharWidth / scaled_font->logical_scale;
	extents.max_y_advance = 0;

    } else {
	/* For all other transformations, we use the design metrics
	 * of the font. The GDI results from GetTextMetrics() on a
	 * transformed font are inexplicably large and we want to
	 * avoid them.
	 */
	status = _cairo_win32_scaled_font_select_unscaled_font (&scaled_font->base, hdc);
	if (status)
	    return status;
	GetTextMetrics (hdc, &metrics);
	_cairo_win32_scaled_font_done_unscaled_font (&scaled_font->base);

	extents.ascent = (double)metrics.tmAscent / scaled_font->em_square;
	extents.descent = (double)metrics.tmDescent / scaled_font->em_square;
	extents.height = (double)(metrics.tmHeight + metrics.tmExternalLeading) / scaled_font->em_square;
	extents.max_x_advance = (double)(metrics.tmMaxCharWidth) / scaled_font->em_square;
	extents.max_y_advance = 0;

    }

    scaled_font->is_truetype = (metrics.tmPitchAndFamily & TMPF_TRUETYPE) != 0;
    scaled_font->glyph_indexing = scaled_font->is_truetype ||
	(GetFontData (hdc, OPENTYPE_CFF_TAG, 0, NULL, 0) != GDI_ERROR);
    // XXX in what situations does this OPENTYPE_CFF thing not have the
    // TMPF_TRUETYPE flag? GetFontData says it only works on Truetype fonts...

    _cairo_scaled_font_set_metrics (&scaled_font->base, &extents);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_win32_scaled_font_init_glyph_metrics (cairo_win32_scaled_font_t *scaled_font,
					     cairo_scaled_glyph_t      *scaled_glyph)
{
    static const MAT2 matrix = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    GLYPHMETRICS metrics;
    cairo_status_t status;
    cairo_text_extents_t extents;
    HDC hdc;

    hdc = _get_global_font_dc ();
    if (!hdc)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (!scaled_font->is_truetype) {
	/* GetGlyphOutline will not work. Assume that the glyph does not extend outside the font box. */
	cairo_font_extents_t font_extents;
	INT width = 0;
	UINT charIndex = _cairo_scaled_glyph_index (scaled_glyph);

	cairo_scaled_font_extents (&scaled_font->base, &font_extents);

	status = cairo_win32_scaled_font_select_font (&scaled_font->base, hdc);
	if (!status) {
	    if (!GetCharWidth32(hdc, charIndex, charIndex, &width)) {
		status = _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_init_glyph_metrics:GetCharWidth32");
		width = 0;
	    }
	}
	cairo_win32_scaled_font_done_font (&scaled_font->base);

	extents.x_bearing = 0;
	extents.y_bearing = -font_extents.ascent / scaled_font->y_scale;
	extents.width = width / scaled_font->x_scale;
	extents.height = (font_extents.ascent + font_extents.descent) / scaled_font->y_scale;
	extents.x_advance = extents.width;
	extents.y_advance = 0;
    } else if (scaled_font->preserve_axes && scaled_font->base.options.hint_style != CAIRO_HINT_METRICS_OFF) {
	/* If we aren't rotating / skewing the axes, then we get the metrics
	 * from the GDI in device space and convert to font space.
	 */
	status = cairo_win32_scaled_font_select_font (&scaled_font->base, hdc);
	if (status)
	    return status;
	if (GetGlyphOutlineW (hdc, _cairo_scaled_glyph_index (scaled_glyph),
			      GGO_METRICS | GGO_GLYPH_INDEX,
			      &metrics, 0, NULL, &matrix) == GDI_ERROR) {
	  status = _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_init_glyph_metrics:GetGlyphOutlineW");
	  memset (&metrics, 0, sizeof (GLYPHMETRICS));
	}
	cairo_win32_scaled_font_done_font (&scaled_font->base);

	if (scaled_font->swap_axes) {
	    extents.x_bearing = - metrics.gmptGlyphOrigin.y / scaled_font->y_scale;
	    extents.y_bearing = metrics.gmptGlyphOrigin.x / scaled_font->x_scale;
	    extents.width = metrics.gmBlackBoxY / scaled_font->y_scale;
	    extents.height = metrics.gmBlackBoxX / scaled_font->x_scale;
	    extents.x_advance = metrics.gmCellIncY / scaled_font->x_scale;
	    extents.y_advance = metrics.gmCellIncX / scaled_font->y_scale;
	} else {
	    extents.x_bearing = metrics.gmptGlyphOrigin.x / scaled_font->x_scale;
	    extents.y_bearing = - metrics.gmptGlyphOrigin.y / scaled_font->y_scale;
	    extents.width = metrics.gmBlackBoxX / scaled_font->x_scale;
	    extents.height = metrics.gmBlackBoxY / scaled_font->y_scale;
	    extents.x_advance = metrics.gmCellIncX / scaled_font->x_scale;
	    extents.y_advance = metrics.gmCellIncY / scaled_font->y_scale;
	}

	if (scaled_font->swap_x) {
	    extents.x_bearing = (- extents.x_bearing - extents.width);
	    extents.x_advance = - extents.x_advance;
	}

	if (scaled_font->swap_y) {
	    extents.y_bearing = (- extents.y_bearing - extents.height);
	    extents.y_advance = - extents.y_advance;
	}

    } else {
	/* For all other transformations, we use the design metrics
	 * of the font.
	 */
	status = _cairo_win32_scaled_font_select_unscaled_font (&scaled_font->base, hdc);
	if (GetGlyphOutlineW (hdc, _cairo_scaled_glyph_index (scaled_glyph),
	                      GGO_METRICS | GGO_GLYPH_INDEX,
			      &metrics, 0, NULL, &matrix) == GDI_ERROR) {
	  status = _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_init_glyph_metrics:GetGlyphOutlineW");
	  memset (&metrics, 0, sizeof (GLYPHMETRICS));
	}
	_cairo_win32_scaled_font_done_unscaled_font (&scaled_font->base);

	extents.x_bearing = (double)metrics.gmptGlyphOrigin.x / scaled_font->em_square;
	extents.y_bearing = - (double)metrics.gmptGlyphOrigin.y / scaled_font->em_square;
	extents.width = (double)metrics.gmBlackBoxX / scaled_font->em_square;
	extents.height = (double)metrics.gmBlackBoxY / scaled_font->em_square;
	extents.x_advance = (double)metrics.gmCellIncX / scaled_font->em_square;
	extents.y_advance = (double)metrics.gmCellIncY / scaled_font->em_square;
    }

    _cairo_scaled_glyph_set_metrics (scaled_glyph,
				     &scaled_font->base,
				     &extents);

    return CAIRO_STATUS_SUCCESS;
}

/* Not currently used code, but may be useful in the future if we add
 * back the capability to the scaled font backend interface to get the
 * actual device space bbox rather than computing it from the
 * font-space metrics.
 */
#if 0
static cairo_status_t
_cairo_win32_scaled_font_glyph_bbox (void		 *abstract_font,
				     const cairo_glyph_t *glyphs,
				     int                  num_glyphs,
				     cairo_box_t         *bbox)
{
    static const MAT2 matrix = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    cairo_win32_scaled_font_t *scaled_font = abstract_font;
    int x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    if (num_glyphs > 0) {
	HDC hdc = _get_global_font_dc ();
	GLYPHMETRICS metrics;
	cairo_status_t status;
	int i;
        UINT glyph_index_option;

	if (!hdc)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	status = cairo_win32_scaled_font_select_font (&scaled_font->base, hdc);
	if (status)
	    return status;

        if (scaled_font->glyph_indexing)
            glyph_index_option = GGO_GLYPH_INDEX;
        else
            glyph_index_option = 0;

	for (i = 0; i < num_glyphs; i++) {
	    int x = _cairo_lround (glyphs[i].x);
	    int y = _cairo_lround (glyphs[i].y);

	    GetGlyphOutlineW (hdc, glyphs[i].index, GGO_METRICS | glyph_index_option,
			     &metrics, 0, NULL, &matrix);

	    if (i == 0 || x1 > x + metrics.gmptGlyphOrigin.x)
		x1 = x + metrics.gmptGlyphOrigin.x;
	    if (i == 0 || y1 > y - metrics.gmptGlyphOrigin.y)
		y1 = y - metrics.gmptGlyphOrigin.y;
	    if (i == 0 || x2 < x + metrics.gmptGlyphOrigin.x + (int)metrics.gmBlackBoxX)
		x2 = x + metrics.gmptGlyphOrigin.x + (int)metrics.gmBlackBoxX;
	    if (i == 0 || y2 < y - metrics.gmptGlyphOrigin.y + (int)metrics.gmBlackBoxY)
		y2 = y - metrics.gmptGlyphOrigin.y + (int)metrics.gmBlackBoxY;
	}

	cairo_win32_scaled_font_done_font (&scaled_font->base);
    }

    bbox->p1.x = _cairo_fixed_from_int (x1);
    bbox->p1.y = _cairo_fixed_from_int (y1);
    bbox->p2.x = _cairo_fixed_from_int (x2);
    bbox->p2.y = _cairo_fixed_from_int (y2);

    return CAIRO_STATUS_SUCCESS;
}
#endif

typedef struct {
    cairo_win32_scaled_font_t *scaled_font;
    HDC hdc;

    cairo_array_t glyphs;
    cairo_array_t dx;

    int start_x;
    int last_x;
    int last_y;
} cairo_glyph_state_t;

static void
_start_glyphs (cairo_glyph_state_t        *state,
	       cairo_win32_scaled_font_t  *scaled_font,
	       HDC                         hdc)
{
    state->hdc = hdc;
    state->scaled_font = scaled_font;

    _cairo_array_init (&state->glyphs, sizeof (WCHAR));
    _cairo_array_init (&state->dx, sizeof (int));
}

static cairo_status_t
_flush_glyphs (cairo_glyph_state_t *state)
{
    cairo_status_t status;
    int dx = 0;
    WCHAR * elements;
    int * dx_elements;
    UINT glyph_index_option;

    status = _cairo_array_append (&state->dx, &dx);
    if (status)
	return status;

    if (state->scaled_font->glyph_indexing)
        glyph_index_option = ETO_GLYPH_INDEX;
    else
        glyph_index_option = 0;

    elements = _cairo_array_index (&state->glyphs, 0);
    dx_elements = _cairo_array_index (&state->dx, 0);
    if (!ExtTextOutW (state->hdc,
		      state->start_x, state->last_y,
		      glyph_index_option,
		      NULL,
		      elements,
		      state->glyphs.num_elements,
		      dx_elements)) {
	return _cairo_win32_print_gdi_error ("_flush_glyphs");
    }

    _cairo_array_truncate (&state->glyphs, 0);
    _cairo_array_truncate (&state->dx, 0);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_add_glyph (cairo_glyph_state_t *state,
	    unsigned long        index,
	    double               device_x,
	    double               device_y)
{
    cairo_status_t status;
    double user_x = device_x;
    double user_y = device_y;
    WCHAR glyph_index = index;
    int logical_x, logical_y;

    cairo_matrix_transform_point (&state->scaled_font->device_to_logical, &user_x, &user_y);

    logical_x = _cairo_lround (user_x);
    logical_y = _cairo_lround (user_y);

    if (state->glyphs.num_elements > 0) {
	int dx;

	if (logical_y != state->last_y) {
	    status = _flush_glyphs (state);
	    if (status)
		return status;
	    state->start_x = logical_x;
	}

	dx = logical_x - state->last_x;
	status = _cairo_array_append (&state->dx, &dx);
	if (status)
	    return status;
    } else {
	state->start_x = logical_x;
    }

    state->last_x = logical_x;
    state->last_y = logical_y;

    status = _cairo_array_append (&state->glyphs, &glyph_index);
    if (status)
	return status;

    return CAIRO_STATUS_SUCCESS;
}

static void
_finish_glyphs (cairo_glyph_state_t *state)
{
    /* ignore errors as we only call _finish_glyphs on the error path */
    _flush_glyphs (state);

    _cairo_array_fini (&state->glyphs);
    _cairo_array_fini (&state->dx);
}

static cairo_status_t
_draw_glyphs_on_surface (cairo_win32_surface_t     *surface,
			 cairo_win32_scaled_font_t *scaled_font,
			 COLORREF                   color,
			 int                        x_offset,
			 int                        y_offset,
			 const cairo_glyph_t       *glyphs,
			 int                 	    num_glyphs)
{
    cairo_glyph_state_t state;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    int i;

    if (!SaveDC (surface->dc))
	return _cairo_win32_print_gdi_error ("_draw_glyphs_on_surface:SaveDC");

    status = cairo_win32_scaled_font_select_font (&scaled_font->base, surface->dc);
    if (status)
	goto FAIL1;

    SetTextColor (surface->dc, color);
    SetTextAlign (surface->dc, TA_BASELINE | TA_LEFT);
    SetBkMode (surface->dc, TRANSPARENT);

    _start_glyphs (&state, scaled_font, surface->dc);

    for (i = 0; i < num_glyphs; i++) {
	status = _add_glyph (&state, glyphs[i].index,
			     glyphs[i].x - x_offset, glyphs[i].y - y_offset);
	if (status)
	    goto FAIL2;
    }

 FAIL2:
    _finish_glyphs (&state);
    cairo_win32_scaled_font_done_font (&scaled_font->base);
 FAIL1:
    RestoreDC (surface->dc, -1);

    return status;
}

/* Duplicate the green channel of a 4-channel mask in the alpha channel, then
 * invert the whole mask.
 */
static void
_compute_argb32_mask_alpha (cairo_win32_surface_t *mask_surface)
{
    cairo_image_surface_t *image = (cairo_image_surface_t *)mask_surface->image;
    int i, j;

    for (i = 0; i < image->height; i++) {
	uint32_t *p = (uint32_t *) (image->data + i * image->stride);
	for (j = 0; j < image->width; j++) {
	    *p = 0xffffffff ^ (*p | ((*p & 0x0000ff00) << 16));
	    p++;
	}
    }
}

/* Invert a mask
 */
static void
_invert_argb32_mask (cairo_win32_surface_t *mask_surface)
{
    cairo_image_surface_t *image = (cairo_image_surface_t *)mask_surface->image;
    int i, j;

    for (i = 0; i < image->height; i++) {
	uint32_t *p = (uint32_t *) (image->data + i * image->stride);
	for (j = 0; j < image->width; j++) {
	    *p = 0xffffffff ^ *p;
	    p++;
	}
    }
}

/* Compute an alpha-mask from a monochrome RGB24 image
 */
static cairo_surface_t *
_compute_a8_mask (cairo_win32_surface_t *mask_surface)
{
    cairo_image_surface_t *image24 = (cairo_image_surface_t *)mask_surface->image;
    cairo_image_surface_t *image8;
    int i, j;

    image8 = (cairo_image_surface_t *)cairo_image_surface_create (CAIRO_FORMAT_A8,
								  image24->width, image24->height);
    if (image8->base.status)
	return NULL;

    for (i = 0; i < image24->height; i++) {
	uint32_t *p = (uint32_t *) (image24->data + i * image24->stride);
	unsigned char *q = (unsigned char *) (image8->data + i * image8->stride);

	for (j = 0; j < image24->width; j++) {
	    *q = 255 - ((*p & 0x0000ff00) >> 8);
	    p++;
	    q++;
	}
    }

    return &image8->base;
}

static cairo_int_status_t
_cairo_win32_scaled_font_glyph_init (void		       *abstract_font,
				     cairo_scaled_glyph_t      *scaled_glyph,
				     cairo_scaled_glyph_info_t  info)
{
    cairo_win32_scaled_font_t *scaled_font = abstract_font;
    cairo_status_t status;

    if ((info & CAIRO_SCALED_GLYPH_INFO_METRICS) != 0) {
	status = _cairo_win32_scaled_font_init_glyph_metrics (scaled_font, scaled_glyph);
	if (status)
	    return status;
    }

    if ((info & CAIRO_SCALED_GLYPH_INFO_SURFACE) != 0) {
	ASSERT_NOT_REACHED;
    }

    if ((info & CAIRO_SCALED_GLYPH_INFO_PATH) != 0) {
	status = _cairo_win32_scaled_font_init_glyph_path (scaled_font, scaled_glyph);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_win32_scaled_font_show_glyphs (void		       *abstract_font,
				      cairo_operator_t    	op,
				      cairo_pattern_t          *pattern,
				      cairo_surface_t          *generic_surface,
				      int                 	source_x,
				      int                 	source_y,
				      int			dest_x,
				      int			dest_y,
				      unsigned int		width,
				      unsigned int		height,
				      cairo_glyph_t	       *glyphs,
				      int                 	num_glyphs)
{
    cairo_win32_scaled_font_t *scaled_font = abstract_font;
    cairo_win32_surface_t *surface = (cairo_win32_surface_t *)generic_surface;
    cairo_status_t status;

    if (width == 0 || height == 0)
	return CAIRO_STATUS_SUCCESS;

    if (_cairo_surface_is_win32 (generic_surface) &&
	surface->format == CAIRO_FORMAT_RGB24 &&
	op == CAIRO_OPERATOR_OVER &&
	_cairo_pattern_is_opaque_solid (pattern)) {

	cairo_solid_pattern_t *solid_pattern = (cairo_solid_pattern_t *)pattern;

	/* When compositing OVER on a GDI-understood surface, with a
	 * solid opaque color, we can just call ExtTextOut directly.
	 */
	COLORREF new_color;

	new_color = RGB (((int)solid_pattern->color.red_short) >> 8,
			 ((int)solid_pattern->color.green_short) >> 8,
			 ((int)solid_pattern->color.blue_short) >> 8);

	status = _draw_glyphs_on_surface (surface, scaled_font, new_color,
					  0, 0,
					  glyphs, num_glyphs);

	return status;
    } else {
	/* Otherwise, we need to draw using software fallbacks. We create a mask
	 * surface by drawing the the glyphs onto a DIB, black-on-white then
	 * inverting. GDI outputs gamma-corrected images so inverted black-on-white
	 * is very different from white-on-black. We favor the more common
	 * case where the final output is dark-on-light.
	 */
	cairo_win32_surface_t *tmp_surface;
	cairo_surface_t *mask_surface;
	cairo_surface_pattern_t mask;
	RECT r;

	tmp_surface = (cairo_win32_surface_t *)cairo_win32_surface_create_with_dib (CAIRO_FORMAT_ARGB32, width, height);
	if (tmp_surface->base.status)
	    return tmp_surface->base.status;

	r.left = 0;
	r.top = 0;
	r.right = width;
	r.bottom = height;
	FillRect (tmp_surface->dc, &r, GetStockObject (WHITE_BRUSH));

	status = _draw_glyphs_on_surface (tmp_surface,
		                          scaled_font, RGB (0, 0, 0),
					  dest_x, dest_y,
					  glyphs, num_glyphs);
	if (status) {
	    cairo_surface_destroy (&tmp_surface->base);
	    return status;
	}

	if (scaled_font->quality == CLEARTYPE_QUALITY) {
	    /* For ClearType, we need a 4-channel mask. If we are compositing on
	     * a surface with alpha, we need to compute the alpha channel of
	     * the mask (we just copy the green channel). But for a destination
	     * surface without alpha the alpha channel of the mask is ignored
	     */

	    if (surface->format != CAIRO_FORMAT_RGB24)
		_compute_argb32_mask_alpha (tmp_surface);
	    else
		_invert_argb32_mask (tmp_surface);

	    mask_surface = &tmp_surface->base;

	    /* XXX: Hacky, should expose this in cairo_image_surface */
	    pixman_image_set_component_alpha (((cairo_image_surface_t *)tmp_surface->image)->pixman_image, TRUE);

	} else {
	    mask_surface = _compute_a8_mask (tmp_surface);
	    cairo_surface_destroy (&tmp_surface->base);
	    if (!mask_surface)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

	/* For op == OVER, no-cleartype, a possible optimization here is to
	 * draw onto an intermediate ARGB32 surface and alpha-blend that with the
	 * destination
	 */
	_cairo_pattern_init_for_surface (&mask, mask_surface);

	status = _cairo_surface_composite (op, pattern,
					   &mask.base,
					   &surface->base,
					   source_x, source_y,
					   0, 0,
					   dest_x, dest_y,
					   width, height);

	_cairo_pattern_fini (&mask.base);

	cairo_surface_destroy (mask_surface);

	return status;
    }
}

static cairo_int_status_t
_cairo_win32_scaled_font_load_truetype_table (void	       *abstract_font,
                                             unsigned long      tag,
                                             long               offset,
                                             unsigned char     *buffer,
                                             unsigned long     *length)
{
    HDC hdc;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    cairo_win32_scaled_font_t *scaled_font = abstract_font;
    hdc = _get_global_font_dc ();
    if (!hdc)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    tag = (tag&0x000000ff)<<24 | (tag&0x0000ff00)<<8 | (tag&0x00ff0000)>>8 | (tag&0xff000000)>>24;
    status = _cairo_win32_scaled_font_select_unscaled_font (&scaled_font->base, hdc);

    *length = GetFontData (hdc, tag, offset, buffer, *length);
    if (*length == GDI_ERROR)
        status = CAIRO_INT_STATUS_UNSUPPORTED;

    _cairo_win32_scaled_font_done_unscaled_font (&scaled_font->base);

    return status;
}

static void
_cairo_win32_scaled_font_map_glyphs_to_unicode (void *abstract_font,
                                                      cairo_scaled_font_subset_t *font_subset)
{
    cairo_win32_scaled_font_t *scaled_font = abstract_font;
    unsigned int i;

    if (scaled_font->glyph_indexing)
        return;

    for (i = 0; i < font_subset->num_glyphs; i++)
        font_subset->to_unicode[i] = font_subset->glyphs[i];
}

static void
_cairo_win32_transform_FIXED_to_fixed (cairo_matrix_t *matrix,
                                       FIXED Fx, FIXED Fy,
                                       cairo_fixed_t *fx, cairo_fixed_t *fy)
{
    double x = Fx.value + Fx.fract / 65536.0;
    double y = Fy.value + Fy.fract / 65536.0;
    cairo_matrix_transform_point (matrix, &x, &y);
    *fx = _cairo_fixed_from_double (x);
    *fy = _cairo_fixed_from_double (y);
}

static cairo_status_t
_cairo_win32_scaled_font_init_glyph_path (cairo_win32_scaled_font_t *scaled_font,
					  cairo_scaled_glyph_t      *scaled_glyph)
{
    static const MAT2 matrix = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, -1 } };
    cairo_status_t status;
    GLYPHMETRICS metrics;
    HDC hdc;
    DWORD bytesGlyph;
    unsigned char *buffer, *ptr;
    cairo_path_fixed_t *path;
    cairo_matrix_t transform;
    cairo_fixed_t x, y;
    UINT glyph_index_option;

    hdc = _get_global_font_dc ();
    if (!hdc)
        return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    path = _cairo_path_fixed_create ();
    if (!path)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (scaled_font->base.options.hint_style == CAIRO_HINT_STYLE_NONE) {
        status = _cairo_win32_scaled_font_select_unscaled_font (&scaled_font->base, hdc);
        transform = scaled_font->base.scale;
        cairo_matrix_scale (&transform, 1.0/scaled_font->em_square, 1.0/scaled_font->em_square);
    } else {
        status = cairo_win32_scaled_font_select_font (&scaled_font->base, hdc);
        cairo_matrix_init_identity(&transform);
    }
    if (status)
        goto CLEANUP_PATH;

    if (scaled_font->glyph_indexing)
        glyph_index_option = GGO_GLYPH_INDEX;
    else
        glyph_index_option = 0;

    bytesGlyph = GetGlyphOutlineW (hdc, _cairo_scaled_glyph_index (scaled_glyph),
				   GGO_NATIVE | glyph_index_option,
				   &metrics, 0, NULL, &matrix);

    if (bytesGlyph == GDI_ERROR) {
	status = _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_glyph_path");
	goto CLEANUP_FONT;
    }

    ptr = buffer = malloc (bytesGlyph);

    if (!buffer) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto CLEANUP_FONT;
    }

    if (GetGlyphOutlineW (hdc, _cairo_scaled_glyph_index (scaled_glyph),
			  GGO_NATIVE | glyph_index_option,
			  &metrics, bytesGlyph, buffer, &matrix) == GDI_ERROR) {
	status = _cairo_win32_print_gdi_error ("_cairo_win32_scaled_font_glyph_path");
	goto CLEANUP_BUFFER;
    }

    while (ptr < buffer + bytesGlyph) {
	TTPOLYGONHEADER *header = (TTPOLYGONHEADER *)ptr;
	unsigned char *endPoly = ptr + header->cb;

	ptr += sizeof (TTPOLYGONHEADER);

        _cairo_win32_transform_FIXED_to_fixed (&transform,
                                               header->pfxStart.x,
                                               header->pfxStart.y,
                                               &x, &y);
        status = _cairo_path_fixed_move_to (path, x, y);
	if (status)
	    goto CLEANUP_BUFFER;

	while (ptr < endPoly) {
	    TTPOLYCURVE *curve = (TTPOLYCURVE *)ptr;
	    POINTFX *points = curve->apfx;
	    int i;
	    switch (curve->wType) {
	    case TT_PRIM_LINE:
		for (i = 0; i < curve->cpfx; i++) {
                    _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                           points[i].x,
                                                           points[i].y,
                                                           &x, &y);
		    status = _cairo_path_fixed_line_to (path, x, y);
		    if (status)
			goto CLEANUP_BUFFER;
		}
		break;
	    case TT_PRIM_QSPLINE:
		for (i = 0; i < curve->cpfx - 1; i++) {
		    cairo_fixed_t p1x, p1y, p2x, p2y, cx, cy, c1x, c1y, c2x, c2y;
		    if (! _cairo_path_fixed_get_current_point (path, &p1x, &p1y))
			goto CLEANUP_BUFFER;
                    _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                           points[i].x,
                                                           points[i].y,
                                                           &cx, &cy);

		    if (i + 1 == curve->cpfx - 1) {
                        _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                               points[i + 1].x,
                                                               points[i + 1].y,
                                                               &p2x, &p2y);
		    } else {
			/* records with more than one curve use interpolation for
			   control points, per http://support.microsoft.com/kb/q87115/ */
                        _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                               points[i + 1].x,
                                                               points[i + 1].y,
                                                               &x, &y);
                        p2x = (cx + x) / 2;
			p2y = (cy + y) / 2;
		    }

		    c1x = 2 * cx / 3 + p1x / 3;
		    c1y = 2 * cy / 3 + p1y / 3;
		    c2x = 2 * cx / 3 + p2x / 3;
		    c2y = 2 * cy / 3 + p2y / 3;

		    status = _cairo_path_fixed_curve_to (path, c1x, c1y, c2x, c2y, p2x, p2y);
		    if (status)
			goto CLEANUP_BUFFER;
		}
		break;
	    case TT_PRIM_CSPLINE:
		for (i = 0; i < curve->cpfx - 2; i += 2) {
		    cairo_fixed_t x1, y1, x2, y2;
                    _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                           points[i].x,
                                                           points[i].y,
                                                           &x, &y);
                    _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                           points[i + 1].x,
                                                           points[i + 1].y,
                                                           &x1, &y1);
                    _cairo_win32_transform_FIXED_to_fixed (&transform,
                                                           points[i + 2].x,
                                                           points[i + 2].y,
                                                           &x2, &y2);
		    status = _cairo_path_fixed_curve_to (path, x, y, x1, y1, x2, y2);
		    if (status)
			goto CLEANUP_BUFFER;
		}
		break;
	    }
	    ptr += sizeof(TTPOLYCURVE) + sizeof (POINTFX) * (curve->cpfx - 1);
	}
	status = _cairo_path_fixed_close_path (path);
	if (status)
	    goto CLEANUP_BUFFER;
    }

    _cairo_scaled_glyph_set_path (scaled_glyph,
				  &scaled_font->base,
				  path);

 CLEANUP_BUFFER:
    free (buffer);

 CLEANUP_FONT:
    cairo_win32_scaled_font_done_font (&scaled_font->base);

 CLEANUP_PATH:
    if (status != CAIRO_STATUS_SUCCESS)
	_cairo_path_fixed_destroy (path);

    return status;
}

const cairo_scaled_font_backend_t cairo_win32_scaled_font_backend = {
    CAIRO_FONT_TYPE_WIN32,
    _cairo_win32_scaled_font_create_toy,
    _cairo_win32_scaled_font_fini,
    _cairo_win32_scaled_font_glyph_init,
    _cairo_win32_scaled_font_text_to_glyphs,
    NULL,			/* ucs4_to_index */
    _cairo_win32_scaled_font_show_glyphs,
    _cairo_win32_scaled_font_load_truetype_table,
    _cairo_win32_scaled_font_map_glyphs_to_unicode,
};

/* cairo_win32_font_face_t */

typedef struct _cairo_win32_font_face cairo_win32_font_face_t;

/* If hfont is non-NULL then logfont->lfHeight must be -S for some S,
 * logfont->lfWidth, logfont->lfEscapement, logfont->lfOrientation must
 * all be 0, and hfont is the result of calling CreateFontIndirectW on
 * logfont.
 */
struct _cairo_win32_font_face {
    cairo_font_face_t base;
    LOGFONTW logfont;
    HFONT hfont;
};

/* implement the platform-specific interface */

static void
_cairo_win32_font_face_destroy (void *abstract_face)
{
}

static cairo_bool_t
_is_scale (const cairo_matrix_t *matrix, double scale)
{
    return matrix->xx == scale && matrix->yy == scale &&
           matrix->xy == 0. && matrix->yx == 0. &&
           matrix->x0 == 0. && matrix->y0 == 0.;
}

static cairo_status_t
_cairo_win32_font_face_scaled_font_create (void			*abstract_face,
					   const cairo_matrix_t	*font_matrix,
					   const cairo_matrix_t	*ctm,
					   const cairo_font_options_t *options,
					   cairo_scaled_font_t **font)
{
    HFONT hfont = NULL;

    cairo_win32_font_face_t *font_face = abstract_face;

    if (font_face->hfont) {
        /* Check whether it's OK to go ahead and use the font-face's HFONT. */
        if (_is_scale (ctm, 1.) &&
            _is_scale (font_matrix, -font_face->logfont.lfHeight)) {
            hfont = font_face->hfont;
        }
    }

    return _win32_scaled_font_create (&font_face->logfont,
				      hfont,
				      &font_face->base,
				      font_matrix, ctm, options,
				      font);
}

static const cairo_font_face_backend_t _cairo_win32_font_face_backend = {
    CAIRO_FONT_TYPE_WIN32,
    _cairo_win32_font_face_destroy,
    _cairo_win32_font_face_scaled_font_create
};

/**
 * cairo_win32_font_face_create_for_logfontw_hfont:
 * @logfont: A #LOGFONTW structure specifying the font to use.
 *   If hfont is null then the lfHeight, lfWidth, lfOrientation and lfEscapement
 *   fields of this structure are ignored. Otherwise lfWidth, lfOrientation and
 *   lfEscapement must be zero.
 * @font: An #HFONT that can be used when the font matrix is a scale by
 *   -lfHeight and the CTM is identity.
 *
 * Creates a new font for the Win32 font backend based on a
 * #LOGFONT. This font can then be used with
 * cairo_set_font_face() or cairo_scaled_font_create().
 * The #cairo_scaled_font_t
 * returned from cairo_scaled_font_create() is also for the Win32 backend
 * and can be used with functions such as cairo_win32_scaled_font_select_font().
 *
 * Return value: a newly created #cairo_font_face_t. Free with
 *  cairo_font_face_destroy() when you are done using it.
 **/
cairo_font_face_t *
cairo_win32_font_face_create_for_logfontw_hfont (LOGFONTW *logfont, HFONT font)
{
    cairo_win32_font_face_t *font_face;

    font_face = malloc (sizeof (cairo_win32_font_face_t));
    if (!font_face) {
        _cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
        return (cairo_font_face_t *)&_cairo_font_face_nil;
    }

    font_face->logfont = *logfont;
    font_face->hfont = font;

    _cairo_font_face_init (&font_face->base, &_cairo_win32_font_face_backend);

    return &font_face->base;
}

/**
 * cairo_win32_font_face_create_for_logfontw:
 * @logfont: A #LOGFONTW structure specifying the font to use.
 *   The lfHeight, lfWidth, lfOrientation and lfEscapement
 *   fields of this structure are ignored.
 *
 * Creates a new font for the Win32 font backend based on a
 * #LOGFONT. This font can then be used with
 * cairo_set_font_face() or cairo_scaled_font_create().
 * The #cairo_scaled_font_t
 * returned from cairo_scaled_font_create() is also for the Win32 backend
 * and can be used with functions such as cairo_win32_scaled_font_select_font().
 *
 * Return value: a newly created #cairo_font_face_t. Free with
 *  cairo_font_face_destroy() when you are done using it.
 **/
cairo_font_face_t *
cairo_win32_font_face_create_for_logfontw (LOGFONTW *logfont)
{
    return cairo_win32_font_face_create_for_logfontw_hfont (logfont, NULL);
}

/**
 * cairo_win32_font_face_create_for_hfont:
 * @font: An #HFONT structure specifying the font to use.
 *
 * Creates a new font for the Win32 font backend based on a
 * #HFONT. This font can then be used with
 * cairo_set_font_face() or cairo_scaled_font_create().
 * The #cairo_scaled_font_t
 * returned from cairo_scaled_font_create() is also for the Win32 backend
 * and can be used with functions such as cairo_win32_scaled_font_select_font().
 *
 * Return value: a newly created #cairo_font_face_t. Free with
 *  cairo_font_face_destroy() when you are done using it.
 **/
cairo_font_face_t *
cairo_win32_font_face_create_for_hfont (HFONT font)
{
    LOGFONTW logfont;
    GetObject (font, sizeof(logfont), &logfont);

    if (logfont.lfEscapement != 0 || logfont.lfOrientation != 0 ||
        logfont.lfWidth != 0) {
        /* We can't use this font because that optimization requires that
         * lfEscapement, lfOrientation and lfWidth be zero. */
        font = NULL;
    }

    return cairo_win32_font_face_create_for_logfontw_hfont (&logfont, font);
}

/**
 * cairo_win32_scaled_font_select_font:
 * @scaled_font: A #cairo_scaled_font_t from the Win32 font backend. Such an
 *   object can be created with cairo_win32_scaled_font_create_for_logfontw().
 * @hdc: a device context
 *
 * Selects the font into the given device context and changes the
 * map mode and world transformation of the device context to match
 * that of the font. This function is intended for use when using
 * layout APIs such as Uniscribe to do text layout with the
 * cairo font. After finishing using the device context, you must call
 * cairo_win32_scaled_font_done_font() to release any resources allocated
 * by this function.
 *
 * See cairo_win32_scaled_font_get_metrics_factor() for converting logical
 * coordinates from the device context to font space.
 *
 * Normally, calls to SaveDC() and RestoreDC() would be made around
 * the use of this function to preserve the original graphics state.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if the operation succeeded.
 *   otherwise an error such as %CAIRO_STATUS_NO_MEMORY and
 *   the device context is unchanged.
 **/
cairo_status_t
cairo_win32_scaled_font_select_font (cairo_scaled_font_t *scaled_font,
				     HDC                  hdc)
{
    cairo_status_t status;
    HFONT hfont;
    HFONT old_hfont = NULL;
    int old_mode;

    if (scaled_font->status)
	return scaled_font->status;

    hfont = _win32_scaled_font_get_scaled_hfont ((cairo_win32_scaled_font_t *)scaled_font);
    if (!hfont)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    old_hfont = SelectObject (hdc, hfont);
    if (!old_hfont)
	return _cairo_win32_print_gdi_error ("cairo_win32_scaled_font_select_font:SelectObject");

    old_mode = SetGraphicsMode (hdc, GM_ADVANCED);
    if (!old_mode) {
	status = _cairo_win32_print_gdi_error ("cairo_win32_scaled_font_select_font:SetGraphicsMode");
	SelectObject (hdc, old_hfont);
	return status;
    }

    status = _win32_scaled_font_set_world_transform ((cairo_win32_scaled_font_t *)scaled_font, hdc);
    if (status) {
	SetGraphicsMode (hdc, old_mode);
	SelectObject (hdc, old_hfont);
	return status;
    }

    SetMapMode (hdc, MM_TEXT);

    return CAIRO_STATUS_SUCCESS;
}

/**
 * cairo_win32_scaled_font_done_font:
 * @scaled_font: A scaled font from the Win32 font backend.
 *
 * Releases any resources allocated by cairo_win32_scaled_font_select_font()
 **/
void
cairo_win32_scaled_font_done_font (cairo_scaled_font_t *scaled_font)
{
}

/**
 * cairo_win32_scaled_font_get_metrics_factor:
 * @scaled_font: a scaled font from the Win32 font backend
 *
 * Gets a scale factor between logical coordinates in the coordinate
 * space used by cairo_win32_scaled_font_select_font() (that is, the
 * coordinate system used by the Windows functions to return metrics) and
 * font space coordinates.
 *
 * Return value: factor to multiply logical units by to get font space
 *               coordinates.
 **/
double
cairo_win32_scaled_font_get_metrics_factor (cairo_scaled_font_t *scaled_font)
{
    return 1. / ((cairo_win32_scaled_font_t *)scaled_font)->logical_scale;
}

/**
 * cairo_win32_scaled_font_get_logical_to_device:
 * @scaled_font: a scaled font from the Win32 font backend
 * @logical_to_device: matrix to return
 *
 * Gets the transformation mapping the logical space used by @scaled_font
 * to device space.
 *
 * Since: 1.4
 **/
void
cairo_win32_scaled_font_get_logical_to_device (cairo_scaled_font_t *scaled_font,
					       cairo_matrix_t *logical_to_device)
{
    cairo_win32_scaled_font_t *win_font = (cairo_win32_scaled_font_t *)scaled_font;
    *logical_to_device = win_font->logical_to_device;
}

/**
 * cairo_win32_scaled_font_get_device_to_logical:
 * @scaled_font: a scaled font from the Win32 font backend
 * @device_to_logical: matrix to return
 *
 * Gets the transformation mapping device space to the logical space
 * used by @scaled_font.
 *
 * Since: 1.4
 **/
void
cairo_win32_scaled_font_get_device_to_logical (cairo_scaled_font_t *scaled_font,
					       cairo_matrix_t *device_to_logical)
{
    cairo_win32_scaled_font_t *win_font = (cairo_win32_scaled_font_t *)scaled_font;
    *device_to_logical = win_font->device_to_logical;
}
