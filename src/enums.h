#ifndef __SP_ENUMS_H__
#define __SP_ENUMS_H__

/*
 * Main program enumerated types
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/* preserveAspectRatio */

enum {
	SP_ASPECT_NONE,
	SP_ASPECT_XMIN_YMIN,
	SP_ASPECT_XMID_YMIN,
	SP_ASPECT_XMAX_YMIN,
	SP_ASPECT_XMIN_YMID,
	SP_ASPECT_XMID_YMID,
	SP_ASPECT_XMAX_YMID,
	SP_ASPECT_XMIN_YMAX,
	SP_ASPECT_XMID_YMAX,
	SP_ASPECT_XMAX_YMAX
};

enum {
	SP_ASPECT_MEET,
	SP_ASPECT_SLICE
};

/* maskUnits */
/* maskContentUnits */

enum {
	SP_CONTENT_UNITS_USERSPACEONUSE,
	SP_CONTENT_UNITS_OBJECTBOUNDINGBOX
};

/* markerUnits */

enum {
	SP_MARKER_UNITS_STROKEWIDTH,
	SP_MARKER_UNITS_USERSPACEONUSE
};

/* stroke-linejoin */
/* stroke-linecap */

/* markers */

enum {
  SP_MARKER_NONE,
  SP_MARKER_TRIANGLE,
  SP_MARKER_ARROW,
};

/* fill-rule */
/* clip-rule */

#endif

