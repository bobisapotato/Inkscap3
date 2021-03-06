#ifndef __SP_POLYGON_H__
#define __SP_POLYGON_H__

/*
 * SVG <polygon> implementation
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib.h>
#include "sp-shape.h"

#define SP_TYPE_POLYGON (sp_polygon_get_type ())
#define SP_POLYGON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SP_TYPE_POLYGON, SPPolygon))
#define SP_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SP_TYPE_POLYGON, SPPolygonClass))
#define SP_IS_POLYGON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SP_TYPE_POLYGON))
#define SP_IS_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SP_TYPE_POLYGON))

struct _SPPolygon {
	SPShape shape;
};

struct _SPPolygonClass {
	SPShapeClass parent_class;
};

GType sp_polygon_get_type (void);

#endif
