#define __SP_STROKE_STYLE_C__

/*
 * Stroke style dialog
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Bryce Harrington <brycehar@bryceharrington.com>
 *
 * Copyright (C) 2001-2002 authors
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noSP_SS_VERBOSE

#include <config.h>

#include <string.h>
#include <glib.h>

#include <libnr/nr-values.h>
#include <libnr/nr-matrix.h>

#include <gtk/gtksignal.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkmisc.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktable.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkiconfactory.h>

#include "helper/sp-intl.h"
#include "helper/unit-menu.h"
#include "../svg/svg.h"
#include "../widgets/sp-widget.h"
#include "../widgets/spw-utilities.h"
#include "../sp-gradient.h"
#include <widgets/paint-selector.h>
#include <widgets/dash-selector.h>
#include "enums.h"
#include "style.h"
#include "../gradient-chemistry.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../sp-item.h"
#include "../inkscape.h"
#include "../inkscape-stock.h"
#include "dialog-events.h"

#include "stroke-style.h"

/* Paint */

static void sp_stroke_style_paint_construct (SPWidget *spw, SPPaintSelector *psel);
static void sp_stroke_style_paint_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, SPPaintSelector *psel);
static void sp_stroke_style_paint_change_selection (SPWidget *spw, SPSelection *selection, SPPaintSelector *psel);
static void sp_stroke_style_paint_attr_changed (SPWidget *spw, const gchar *key, const gchar *oldval, const gchar *newval);
static void sp_stroke_style_paint_update (SPWidget *spw, SPSelection *sel);
static void sp_stroke_style_paint_update_repr (SPWidget *spw, SPRepr *repr);

static void sp_stroke_style_paint_mode_changed (SPPaintSelector *psel, SPPaintSelectorMode mode, SPWidget *spw);
static void sp_stroke_style_paint_dragged (SPPaintSelector *psel, SPWidget *spw);
static void sp_stroke_style_paint_changed (SPPaintSelector *psel, SPWidget *spw);

static void sp_stroke_style_get_average_color_rgba (const GSList *objects, gfloat *c);
static void sp_stroke_style_get_average_color_cmyka (const GSList *objects, gfloat *c);
static SPPaintSelectorMode sp_stroke_style_determine_paint_selector_mode (SPStyle *style);

GtkWidget *
sp_stroke_style_paint_widget_new (void)
{
	GtkWidget *spw, *psel;

	spw = sp_widget_new_global (INKSCAPE);

	psel = sp_paint_selector_new ();
	gtk_widget_show (psel);
	gtk_container_add (GTK_CONTAINER (spw), psel);
	gtk_object_set_data (GTK_OBJECT (spw), "paint-selector", psel);

	gtk_signal_connect (GTK_OBJECT (spw), "construct", GTK_SIGNAL_FUNC (sp_stroke_style_paint_construct), psel);
	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_stroke_style_paint_modify_selection), psel);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_stroke_style_paint_change_selection), psel);
	gtk_signal_connect (GTK_OBJECT (spw), "attr_changed", GTK_SIGNAL_FUNC (sp_stroke_style_paint_attr_changed), psel);

	gtk_signal_connect (GTK_OBJECT (psel), "mode_changed", GTK_SIGNAL_FUNC (sp_stroke_style_paint_mode_changed), spw);
	gtk_signal_connect (GTK_OBJECT (psel), "dragged", GTK_SIGNAL_FUNC (sp_stroke_style_paint_dragged), spw);
	gtk_signal_connect (GTK_OBJECT (psel), "changed", GTK_SIGNAL_FUNC (sp_stroke_style_paint_changed), spw);

	sp_stroke_style_paint_update (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

void
sp_stroke_style_paint_system_color_set (GtkWidget *widget, SPColor *color, float opacity)
{
	SPPaintSelector *psel;

	psel = SP_PAINT_SELECTOR(g_object_get_data (G_OBJECT (widget), "paint-selector"));

	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_system_color_set (psel, color, opacity);
		break;
	default:
		break;
	}
}

static void
sp_stroke_style_paint_construct (SPWidget *spw, SPPaintSelector *psel)
{
#ifdef SP_SS_VERBOSE
	g_print ("Stroke style widget constructed: inkscape %p repr %p\n", spw->inkscape, spw->repr);
#endif
	if (spw->inkscape) {
		sp_stroke_style_paint_update (spw, SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);
	} else if (spw->repr) {
		sp_stroke_style_paint_update_repr (spw, spw->repr);
	}
}

static void
sp_stroke_style_paint_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, SPPaintSelector *psel)
{
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG)) {
		sp_stroke_style_paint_update (spw, selection);
	}
}

static void
sp_stroke_style_paint_change_selection (SPWidget *spw, SPSelection *selection, SPPaintSelector *psel)
{
	sp_stroke_style_paint_update (spw, selection);
}

static void
sp_stroke_style_paint_attr_changed (SPWidget *spw, const gchar *key, const gchar *oldval, const gchar *newval)
{
	if (!strcmp (key, "style")) {
		/* This sounds interesting */
		sp_stroke_style_paint_update_repr (spw, spw->repr);
	}
}

static void
sp_stroke_style_paint_update (SPWidget *spw, SPSelection *sel)
{
	SPPaintSelector *psel;
	SPPaintSelectorMode pselmode;
	const GSList *objects, *l;
	SPObject *object;
	SPGradient *vector;
	gfloat c[5];
	SPLinearGradient *lg;
	SPRadialGradient *rg;
#if 0
	NRPointF p0, p1;
#endif
	NRMatrixF fctm, gs2d;
	NRRectF fbb;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	psel = SP_PAINT_SELECTOR(gtk_object_get_data (GTK_OBJECT (spw), "paint-selector"));

	if (!sel || sp_selection_is_empty (sel)) {
		/* No objects, set empty */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_EMPTY);
		gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
		return;
	}

	objects = sp_selection_item_list (sel);
	object = SP_OBJECT (objects->data);
	pselmode = sp_stroke_style_determine_paint_selector_mode (SP_OBJECT_STYLE (object));

	for (l = objects->next; l != NULL; l = l->next) {
		SPPaintSelectorMode nextmode;
		nextmode = sp_stroke_style_determine_paint_selector_mode (SP_OBJECT_STYLE (l->data));
		if (nextmode != pselmode) {
			/* Multiple styles */
			sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
			gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
			return;
		}
	}
#ifdef SP_SS_VERBOSE
	g_print ("StrokeStyleWidget: paint selector mode %d\n", pselmode);
#endif
	switch (pselmode) {
	case SP_PAINT_SELECTOR_MODE_NONE:
		/* No paint at all */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_NONE);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_RGB);
		sp_stroke_style_get_average_color_rgba (objects, c);
		sp_paint_selector_set_color_rgba_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_CMYK);
		sp_stroke_style_get_average_color_cmyka (objects, c);
		sp_paint_selector_set_color_cmyka_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		object = SP_OBJECT (objects->data);
		/* We know that all objects have lineargradient stroke style */
		vector = sp_gradient_get_vector (SP_GRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (object)), FALSE);
		for (l = objects->next; l != NULL; l = l->next) {
			SPObject *next;
			next = SP_OBJECT (l->data);
			if (sp_gradient_get_vector (SP_GRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (next)), FALSE) != vector) {
				/* Multiple vectors */
				sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
				gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
				return;
			}
		}
		/* fixme: Probably we should set multiple mode here too */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR);
		sp_paint_selector_set_gradient_linear (psel, vector);
		sp_selection_bbox_document (sel, &fbb);
		sp_paint_selector_set_gradient_bbox (psel, fbb.x0, fbb.y0, fbb.x1, fbb.y1);
		/* fixme: This is plain wrong */
		lg = SP_LINEARGRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (object));
		sp_item_invoke_bbox (SP_ITEM (object), &fbb, NULL, TRUE);
		sp_item_i2doc_affine (SP_ITEM (object), &fctm);
		sp_gradient_get_gs2d_matrix_f (SP_GRADIENT (lg), &fctm, &fbb, &gs2d);
		sp_paint_selector_set_gradient_gs2d_matrix_f (psel, &gs2d);
		sp_paint_selector_set_gradient_properties (psel, SP_GRADIENT_UNITS (lg), SP_GRADIENT_SPREAD (lg));
		sp_paint_selector_set_lgradient_position (psel, lg->x1.computed, lg->y1.computed, lg->x2.computed, lg->y2.computed);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		object = SP_OBJECT (objects->data);
		/* We know that all objects have radialgradient stroke style */
		vector = sp_gradient_get_vector (SP_GRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (object)), FALSE);
		for (l = objects->next; l != NULL; l = l->next) {
			SPObject *next;
			next = SP_OBJECT (l->data);
			if (sp_gradient_get_vector (SP_GRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (next)), FALSE) != vector) {
				/* Multiple vectors */
				sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
				gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
				return;
			}
		}
		/* fixme: Probably we should set multiple mode here too */
		sp_paint_selector_set_gradient_radial (psel, vector);
		sp_selection_bbox_document (sel, &fbb);
		sp_paint_selector_set_gradient_bbox (psel, fbb.x0, fbb.y0, fbb.x1, fbb.y1);
		/* fixme: This is plain wrong */
		rg = SP_RADIALGRADIENT (SP_OBJECT_STYLE_STROKE_SERVER (object));
		sp_item_invoke_bbox (SP_ITEM (object), &fbb, NULL, TRUE);
		sp_item_i2doc_affine (SP_ITEM (object), &fctm);
		sp_gradient_get_gs2d_matrix_f (SP_GRADIENT (rg), &fctm, &fbb, &gs2d);
		sp_paint_selector_set_gradient_gs2d_matrix_f (psel, &gs2d);
		sp_paint_selector_set_gradient_properties (psel, SP_GRADIENT_UNITS (rg), SP_GRADIENT_SPREAD (rg));
		sp_paint_selector_set_rgradient_position (psel, rg->cx.computed, rg->cy.computed, rg->fx.computed, rg->fy.computed, rg->r.computed);
		break;
	default:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_MULTIPLE);
		break;
	}

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

static void
sp_stroke_style_paint_update_repr (SPWidget *spw, SPRepr *repr)
{
	SPPaintSelector *psel;
	SPPaintSelectorMode pselmode;
	SPStyle *style;
	gfloat c[5];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	psel = SP_PAINT_SELECTOR(gtk_object_get_data (GTK_OBJECT (spw), "paint-selector"));

	style = sp_style_new ();
	sp_style_read_from_repr (style, repr);

	pselmode = sp_stroke_style_determine_paint_selector_mode (style);
#ifdef SP_SS_VERBOSE
	g_print ("StrokeStyleWidget: paint selector mode %d\n", pselmode);
#endif
	switch (pselmode) {
	case SP_PAINT_SELECTOR_MODE_NONE:
		/* No paint at all */
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_NONE);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_RGB);
		sp_color_get_rgb_floatv (&style->stroke.value.color, c);
		c[3] = SP_SCALE24_TO_FLOAT (style->stroke_opacity.value);
		sp_paint_selector_set_color_rgba_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_set_mode (psel, SP_PAINT_SELECTOR_MODE_COLOR_CMYK);
		sp_color_get_cmyk_floatv (&style->stroke.value.color, c);
		c[4] = SP_SCALE24_TO_FLOAT (style->stroke_opacity.value);
		sp_paint_selector_set_color_cmyka_floatv (psel, c);
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		/* fixme: Think about it (Lauris) */
		break;
	default:
		break;
	}

	sp_style_unref (style);

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

static void
sp_stroke_style_paint_mode_changed (SPPaintSelector *psel, SPPaintSelectorMode mode, SPWidget *spw)
{
	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	/* fixme: Does this work? */
	/* fixme: Not really, here we have to get old color back from object */
	/* Instead of relying on paint widget having meaningful colors set */
	sp_stroke_style_paint_changed (psel, spw);
}

static void
sp_stroke_style_paint_dragged (SPPaintSelector *psel, SPWidget *spw)
{
	const GSList *items, *i;
	SPGradient *vector;
	gfloat c[5];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;
#ifdef SP_SS_VERBOSE
	g_print ("StrokeStyleWidget: paint dragged\n");
#endif
	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_EMPTY:
	case SP_PAINT_SELECTOR_MODE_MULTIPLE:
	case SP_PAINT_SELECTOR_MODE_NONE:
		g_warning ("file %s: line %d: Paint %d should not emit 'dragged'", __FILE__, __LINE__, psel->mode);
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		sp_paint_selector_get_rgba_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_stroke_color_rgba (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], TRUE, TRUE);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		sp_paint_selector_get_cmyka_floatv (psel, c);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			sp_style_set_stroke_color_cmyka (SP_OBJECT_STYLE (i->data), c[0], c[1], c[2], c[3], c[4], TRUE, TRUE);
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		vector = sp_paint_selector_get_gradient_vector (psel);
		vector = sp_gradient_ensure_vector_normalized (vector);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			SPGradient *lg;
			lg = sp_item_force_stroke_lineargradient_vector (SP_ITEM (i->data), vector);
			sp_paint_selector_write_lineargradient (psel, SP_LINEARGRADIENT (lg), SP_ITEM (i->data));
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		vector = sp_paint_selector_get_gradient_vector (psel);
		vector = sp_gradient_ensure_vector_normalized (vector);
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			SPGradient *rg;
			rg = sp_item_force_stroke_radialgradient_vector (SP_ITEM (i->data), vector);
			sp_paint_selector_write_radialgradient (psel, SP_RADIALGRADIENT (rg), SP_ITEM (i->data));
		}
		break;
	default:
		g_warning ("file %s: line %d: Paint selector should not be in mode %d", __FILE__, __LINE__, psel->mode);
		break;
	}
}

static void
sp_stroke_style_paint_changed (SPPaintSelector *psel, SPWidget *spw)
{
	const GSList *items, *i, *r;
	GSList *reprs;
	SPCSSAttr *css;
	gfloat rgba[4], cmyka[5];
	SPGradient *vector;
	gchar b[64];

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;
#ifdef SP_SS_VERBOSE
	g_print ("StrokeStyleWidget: paint changed\n");
#endif
	if (spw->inkscape) {
		/* fixme: */
		if (!SP_WIDGET_DOCUMENT (spw)) return;
		reprs = NULL;
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			reprs = g_slist_prepend (reprs, SP_OBJECT_REPR (i->data));
		}
	} else {
		reprs = g_slist_prepend (NULL, spw->repr);
		items = NULL;
	}

	switch (psel->mode) {
	case SP_PAINT_SELECTOR_MODE_EMPTY:
	case SP_PAINT_SELECTOR_MODE_MULTIPLE:
		g_warning ("file %s: line %d: Paint %d should not emit 'changed'", __FILE__, __LINE__, psel->mode);
		break;
	case SP_PAINT_SELECTOR_MODE_NONE:
		css = sp_repr_css_attr_new ();
		sp_repr_css_set_property (css, "stroke", "none");
		for (r = reprs; r != NULL; r = r->next) {
			sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
			sp_repr_set_attr_recursive ((SPRepr *) r->data, "sodipodi:stroke-cmyk", NULL);
		}
		sp_repr_css_attr_unref (css);
		if (spw->inkscape) sp_document_done (SP_WIDGET_DOCUMENT (spw));
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_RGB:
		css = sp_repr_css_attr_new ();
		sp_paint_selector_get_rgba_floatv (psel, rgba);
		sp_svg_write_color (b, 64, SP_RGBA32_F_COMPOSE (rgba[0], rgba[1], rgba[2], 0.0));
		sp_repr_css_set_property (css, "stroke", b);
		g_snprintf (b, 64, "%g", rgba[3]);
		sp_repr_css_set_property (css, "stroke-opacity", b);
		for (r = reprs; r != NULL; r = r->next) {
			sp_repr_set_attr_recursive ((SPRepr *) r->data, "sodipodi:stroke-cmyk", NULL);
			sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
		}
		sp_repr_css_attr_unref (css);
		if (spw->inkscape) sp_document_done (SP_WIDGET_DOCUMENT (spw));
		break;
	case SP_PAINT_SELECTOR_MODE_COLOR_CMYK:
		css = sp_repr_css_attr_new ();
		sp_paint_selector_get_cmyka_floatv (psel, cmyka);
		sp_color_cmyk_to_rgb_floatv (rgba, cmyka[0], cmyka[1], cmyka[2], cmyka[3]);
		sp_svg_write_color (b, 64, SP_RGBA32_F_COMPOSE (rgba[0], rgba[1], rgba[2], 0.0));
		sp_repr_css_set_property (css, "stroke", b);
		g_snprintf (b, 64, "%g", cmyka[4]);
		sp_repr_css_set_property (css, "stroke-opacity", b);
		g_snprintf (b, 64, "(%g %g %g %g)", cmyka[0], cmyka[1], cmyka[2], cmyka[3]);
		for (r = reprs; r != NULL; r = r->next) {
			sp_repr_set_attr_recursive ((SPRepr *) r->data, "sodipodi:stroke-cmyk", b);
			sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
		}
		sp_repr_css_attr_unref (css);
		if (spw->inkscape) sp_document_done (SP_WIDGET_DOCUMENT (spw));
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR:
		if (items) {
			vector = sp_paint_selector_get_gradient_vector (psel);
			if (!vector) {
				/* No vector in paint selector should mean that we just changed mode */
				vector = sp_document_default_gradient_vector (SP_WIDGET_DOCUMENT (spw));
				for (i = items; i != NULL; i = i->next) {
					sp_item_force_stroke_lineargradient_vector (SP_ITEM (i->data), vector);
				}
			} else {
				vector = sp_gradient_ensure_vector_normalized (vector);
				for (i = items; i != NULL; i = i->next) {
					SPGradient *lg;
					lg = sp_item_force_stroke_lineargradient_vector (SP_ITEM (i->data), vector);
					sp_paint_selector_write_lineargradient (psel, SP_LINEARGRADIENT (lg), SP_ITEM (i->data));
					sp_object_invoke_write (SP_OBJECT (lg), SP_OBJECT_REPR (lg), SP_OBJECT_WRITE_EXT);
				}
			}
			sp_document_done (SP_WIDGET_DOCUMENT (spw));
		}
		break;
	case SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL:
		if (items) {
			vector = sp_paint_selector_get_gradient_vector (psel);
			if (!vector) {
				/* No vector in paint selector should mean that we just changed mode */
				vector = sp_document_default_gradient_vector (SP_WIDGET_DOCUMENT (spw));
				for (i = items; i != NULL; i = i->next) {
					sp_item_force_stroke_radialgradient_vector (SP_ITEM (i->data), vector);
				}
			} else {
				vector = sp_gradient_ensure_vector_normalized (vector);
				for (i = items; i != NULL; i = i->next) {
					SPGradient *lg;
					lg = sp_item_force_stroke_radialgradient_vector (SP_ITEM (i->data), vector);
					sp_paint_selector_write_radialgradient (psel, SP_RADIALGRADIENT (lg), SP_ITEM (i->data));
					sp_object_invoke_write (SP_OBJECT (lg), SP_OBJECT_REPR (lg), SP_OBJECT_WRITE_EXT);
				}
			}
			sp_document_done (SP_WIDGET_DOCUMENT (spw));
		}
		break;
	default:
		g_warning ("file %s: line %d: Paint selector should not be in mode %d", __FILE__, __LINE__, psel->mode);
		break;
	}

	g_slist_free (reprs);
}

/* Line */

static void sp_stroke_style_line_construct (SPWidget *spw, gpointer data);
static void sp_stroke_style_line_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data);
static void sp_stroke_style_line_change_selection (SPWidget *spw, SPSelection *selection, gpointer data);
static void sp_stroke_style_line_attr_changed (SPWidget *spw, const gchar *key, const gchar *oldval, const gchar *newval);

static void sp_stroke_style_line_update (SPWidget *spw, SPSelection *sel);
static void sp_stroke_style_line_update_repr (SPWidget *spw, SPRepr *repr);

static void sp_stroke_style_set_join_buttons (SPWidget *spw, GtkWidget *active);
static void sp_stroke_style_set_cap_buttons (SPWidget *spw, GtkWidget *active);
static void sp_stroke_style_set_marker_buttons (SPWidget *spw, GtkWidget *active);
static void sp_stroke_style_width_changed (GtkAdjustment *adj, SPWidget *spw);
static void sp_stroke_style_any_toggled (GtkToggleButton *tb, SPWidget *spw);
static void sp_stroke_style_line_dash_changed (SPDashSelector *dsel, SPWidget *spw);


/**
 * Helper function for creating radio buttons.  This should probably be re-thought out
 * when reimplementing this with Gtkmm.  
 */
GtkWidget*
sp_stroke_radio_button(GtkWidget* tb, const char* n, const char* xpm,
		       GtkWidget* hb, GtkWidget* spw,
		       const gchar* key, const gchar* data) {
  GtkWidget *px;

  g_assert(xpm != NULL);
  g_assert(hb  != NULL);
  g_assert(spw != NULL);
  
  if (tb == NULL) {
    tb = gtk_radio_button_new (NULL);
  } else {
    tb = gtk_radio_button_new (gtk_radio_button_group (GTK_RADIO_BUTTON (tb)));
  }
  gtk_widget_show (tb);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (tb), FALSE);
  gtk_box_pack_start (GTK_BOX (hb), tb, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (spw), n, tb);
  gtk_object_set_data (GTK_OBJECT (tb), key, (gpointer*)data);
  gtk_signal_connect (GTK_OBJECT (tb), "toggled",
		      GTK_SIGNAL_FUNC (sp_stroke_style_any_toggled),
		      spw);
  px = gtk_image_new_from_file (xpm);
  g_assert(px != NULL);
  gtk_widget_show (px);
  gtk_container_add (GTK_CONTAINER (tb), px);

  return (tb);
}


/**
 * Creates a new widget for the line stroke style.
 */
GtkWidget *
sp_stroke_style_line_widget_new (void)
{
	GtkWidget *spw, *f, *t, *l, *hb, *sb, *us, *tb, *ds;
	GtkObject *a;
	gint i;

	spw = sp_widget_new_global (INKSCAPE);

	f = gtk_frame_new (_("Stroke settings"));
	gtk_widget_show (f);
	gtk_container_set_border_width (GTK_CONTAINER (f), 4);
	gtk_container_add (GTK_CONTAINER (spw), f);
	
	t = gtk_table_new (3, 6, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_container_add (GTK_CONTAINER (f), t);
	gtk_object_set_data (GTK_OBJECT (spw), "stroke", t);

	i=0;

	/* Stroke width */
	spw_label(t, _("Width:"), 0, i);

	hb = spw_hbox(t, 3, 1, i);

	a = gtk_adjustment_new (1.0, 0.0, 100.0, 0.1, 10.0, 10.0);
	gtk_object_set_data (GTK_OBJECT (spw), "width", a);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 2);
	gtk_widget_show (sb);

	sp_dialog_defocus_on_enter (sb); 

	gtk_box_pack_start (GTK_BOX (hb), sb, TRUE, TRUE, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	gtk_widget_show (us);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	gtk_box_pack_start (GTK_BOX (hb), us, FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "units", us);

	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_stroke_style_width_changed), spw);
	i++;

	/* Join type */
	spw_label(t, _("Join:"), 0, i);

	hb = spw_hbox(t, 3, 1, i);

	tb = NULL;

	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_JOIN_MITER,
			       INKSCAPE_GLADEDIR "/join_miter.xpm",
			       hb, spw, "join", "miter");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_JOIN_ROUND,
			       INKSCAPE_GLADEDIR "/join_round.xpm",
			       hb, spw, "join", "round");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_JOIN_BEVEL,
			       INKSCAPE_GLADEDIR "/join_bevel.xpm",
			       hb, spw, "join", "bevel");
	i++;  

	/* Cap type */
	spw_label(t, _("Cap:"), 0, i);

	hb = spw_hbox(t, 3, 1, i);

	tb = NULL;
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_CAP_BUTT,
			       INKSCAPE_GLADEDIR "/cap_butt.xpm",
			       hb, spw, "cap", "butt");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_CAP_ROUND,
			       INKSCAPE_GLADEDIR "/cap_round.xpm",
			       hb, spw, "cap", "round");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_CAP_SQUARE,
			       INKSCAPE_GLADEDIR "/cap_square.xpm",
			       hb, spw, "cap", "square");
	i++;
  

	/* Dash */
	spw_label(t, _("Pattern:"), 0, i);

	ds = sp_dash_selector_new (inkscape_get_repr (INKSCAPE, "palette.dashes"));
	gtk_widget_show (ds);
	gtk_table_attach (GTK_TABLE (t), ds, 1, 4, i, i+1, (GtkAttachOptions)( GTK_EXPAND | GTK_FILL ), (GtkAttachOptions)0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "dash", ds);
	gtk_signal_connect (GTK_OBJECT (ds), "changed", GTK_SIGNAL_FUNC (sp_stroke_style_line_dash_changed), spw);
	i++;

#ifdef MARKERS
	/* Start Marker */
	spw_label(t, _("Start Markers:"), 0, i);
	hb = spw_hbox(t, 3, 1, i);
	g_assert(hb != NULL);
	
	tb = NULL;
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_START_NONE,
			       INKSCAPE_GLADEDIR "/cap_butt.xpm",
			       hb, spw, "start_marker", "none");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_START_FILLED_ARROW,
			       INKSCAPE_GLADEDIR "/cap_round.xpm",
			       hb, spw, "start_marker", "mTriangle");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_START_HOLLOW_ARROW,
			       INKSCAPE_GLADEDIR "/cap_square.xpm",
			       hb, spw, "start_marker", "mArrow");
	i++;

	/* Mid Marker */
	spw_label(t, _("Mid Markers:"), 0, i);
	hb = spw_hbox(t, 3, 1, i);
	g_assert(hb != NULL);
	
	tb = NULL;
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_MID_NONE,
			       INKSCAPE_GLADEDIR "/cap_butt.xpm",
			       hb, spw, "mid_marker", "none");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_MID_FILLED_ARROW,
			       INKSCAPE_GLADEDIR "/cap_round.xpm",
			       hb, spw, "mid_marker", "mTriangle");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_MID_HOLLOW_ARROW,
			       INKSCAPE_GLADEDIR "/cap_square.xpm",
			       hb, spw, "mid_marker", "mArrow");
	i++;

	/* End Marker */
	spw_label(t, _("End Markers:"), 0, i);
	hb = spw_hbox(t, 3, 1, i);
	g_assert(hb != NULL);
	
	tb = NULL;
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_END_NONE,
			       INKSCAPE_GLADEDIR "/cap_butt.xpm",
			       hb, spw, "end_marker", "none");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_END_FILLED_ARROW,
			       INKSCAPE_GLADEDIR "/cap_round.xpm",
			       hb, spw, "end_marker", "mTriangle");
	tb = sp_stroke_radio_button(tb, INKSCAPE_STOCK_END_HOLLOW_ARROW,
			       INKSCAPE_GLADEDIR "/cap_square.xpm",
			       hb, spw, "end_marker", "mArrow");
	i++;
#endif

	/* General (I think) style dialog signals */
	gtk_signal_connect (GTK_OBJECT (spw), "construct", GTK_SIGNAL_FUNC (sp_stroke_style_line_construct), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_stroke_style_line_modify_selection), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_stroke_style_line_change_selection), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "attr_changed", GTK_SIGNAL_FUNC (sp_stroke_style_line_attr_changed), NULL);

	SPDesktop *desktop = inkscape_active_desktop();
	sp_stroke_style_line_update (SP_WIDGET (spw), desktop ? SP_DT_SELECTION (desktop) : NULL);
	// sp_stroke_style_line_update (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

static void
sp_stroke_style_line_construct (SPWidget *spw, gpointer data)
{
#ifdef SP_SS_VERBOSE
	g_print ("Stroke style widget constructed: inkscape %p repr %p\n", spw->inkscape, spw->repr);
#endif
	if (spw->inkscape) {
		sp_stroke_style_line_update (spw, SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);
	} else if (spw->repr) {
		sp_stroke_style_line_update_repr (spw, spw->repr);
	}
}

static void
sp_stroke_style_line_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data)
{
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG)) {
		sp_stroke_style_line_update (spw, selection);
	}
}

static void
sp_stroke_style_line_change_selection (SPWidget *spw, SPSelection *selection, gpointer data)
{
	sp_stroke_style_line_update (spw, selection);
}

static void
sp_stroke_style_line_attr_changed (SPWidget *spw, const gchar *key, const gchar *oldval, const gchar *newval)
{
	if (!strcmp (key, "style")) {
		/* This sounds interesting */
		sp_stroke_style_line_update_repr (spw, spw->repr);
	}
}

static void
sp_stroke_style_line_update (SPWidget *spw, SPSelection *sel)
{
	GtkWidget *sset, *units, *dsel;
	GtkObject *width;
	const SPUnit *unit;
	const GSList *objects, *l;
	SPObject *object;
	SPStyle *style;
	gdouble avgwidth;
	gboolean stroked;
	gboolean joinValid = TRUE;
	unsigned int jointype;
	gboolean capValid = TRUE;
	unsigned int captype;
	GtkWidget *tb;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	sset = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), "stroke"));
	width = GTK_OBJECT(gtk_object_get_data (GTK_OBJECT (spw), "width"));
	units = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), "units"));
	dsel = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), "dash"));

	if (!sel || sp_selection_is_empty (sel)) {
		/* No objects, set empty */
		gtk_widget_set_sensitive (sset, FALSE);
		gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
		return;
	}

	objects = sp_selection_item_list (sel);

	/* Determine average stroke width */
	avgwidth = 0.0;
	stroked = TRUE;
	for (l = objects; l != NULL; l = l->next) {
		NRMatrixF i2d;
		gdouble dist;
		sp_item_i2d_affine (SP_ITEM (l->data), &i2d);
		object = SP_OBJECT (l->data);
		dist = object->style->stroke_width.computed * NR_MATRIX_DF_EXPANSION (&i2d);
#ifdef SP_SS_VERBOSE
		g_print ("%g in user is %g on desktop\n", object->style->stroke_width.computed, dist);
#endif
		avgwidth += dist;
		if (object->style->stroke.type == SP_PAINT_TYPE_NONE) stroked = FALSE;
	}
	if (stroked) {
		gtk_widget_set_sensitive (sset, TRUE);
	} else {
		/* Some objects not stroked, set insensitive */
		gtk_widget_set_sensitive (sset, FALSE);
		gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
		return;
	}
	avgwidth /= g_slist_length ((GSList *) objects);
	unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (units));
	sp_convert_distance (&avgwidth, SP_PS_UNIT, unit);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (width), avgwidth);

	/* Join & Cap */
	object = SP_OBJECT (objects->data);
	style = SP_OBJECT_STYLE (object);
	jointype = object->style->stroke_linejoin.value;
	captype = object->style->stroke_linecap.value;

	for (l = objects->next; l != NULL; l = l->next) {
		SPObject *o;
		o = SP_OBJECT (l->data);
		if (o->style->stroke_linejoin.value != jointype)
		{
			joinValid = FALSE;
		}
		if (o->style->stroke_linecap.value != captype)
		{
			capValid = FALSE;
		}
	}

	tb = NULL;
	if ( joinValid )
	{
		switch (jointype) {
		case SP_STROKE_LINEJOIN_MITER:
			tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_MITER));
			break;
		case SP_STROKE_LINEJOIN_ROUND:
			tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_ROUND));
			break;
		case SP_STROKE_LINEJOIN_BEVEL:
			tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_BEVEL));
			break;
		default:
			break;
		}
	}
	sp_stroke_style_set_join_buttons (spw, tb);

	tb = NULL;
	if ( capValid )
	{
		switch (captype) {
		case SP_STROKE_LINECAP_BUTT:
			tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_BUTT));
			break;
		case SP_STROKE_LINECAP_ROUND:
			tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_ROUND));
			break;
		case SP_STROKE_LINECAP_SQUARE:
			tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_SQUARE));
			break;
		default:
			break;
		}
	}
	sp_stroke_style_set_cap_buttons (spw, tb);

	/* Dash */
	if (style->stroke_dash.n_dash > 0) {
		double d[64];
		int len, i;
		len = MIN (style->stroke_dash.n_dash, 64);
		for (i = 0; i < len; i++) {
			d[i] = style->stroke_dash.dash[i] / style->stroke_width.computed;
		}
		sp_dash_selector_set_dash (SP_DASH_SELECTOR (dsel), len, d, style->stroke_dash.offset / style->stroke_width.computed);
	} else {
		sp_dash_selector_set_dash (SP_DASH_SELECTOR (dsel), 0, NULL, 0.0);
	}

	gtk_widget_set_sensitive (sset, TRUE);

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

/** 
 *  This essentially does a lookup of the string representation of
 *  a given marker's id.  This is intended to be temporary until
 *  a solution with a proper hashmap or lookup table or something
 *  can be written.
 */
const gchar*
marker_id_to_string(unsigned int id) 
{
  switch (id) {          /* A hashmap would be more flexible */
    /*
  case SP_MARKER_NONE:
    return "marker-none";
  case SP_MARKER_TRIANGLE:
    return "marker-triangle";
  case SP_MARKER_ARROW:
    return "marker-arrow";
    */
  default:
    return "error";
  };
}


static void
sp_stroke_style_line_update_repr (SPWidget *spw, SPRepr *repr)
{
	GtkWidget *sset, *units, *dsel;
	GtkObject *width;
	SPStyle *style;
	const SPUnit *unit;
	gdouble swidth;
	GtkWidget *tb;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	sset = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), "stroke"));
	width = GTK_OBJECT(gtk_object_get_data (GTK_OBJECT (spw), "width"));
	units = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), "units"));
	dsel = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), "dash"));

	style = sp_style_new ();
	sp_style_read_from_repr (style, repr);

	if (style->stroke.type == SP_PAINT_TYPE_NONE) {
		gtk_widget_set_sensitive (sset, FALSE);
		gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
		return;
	}

	/* We need points */
	swidth = style->stroke_width.computed / 1.25;
	unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (units));
	sp_convert_distance (&swidth, SP_PS_UNIT, unit);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (width), swidth);

	/* Join & Cap */
	switch (style->stroke_linejoin.value) {
	case SP_STROKE_LINEJOIN_MITER:
		tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_MITER));
		break;
	case SP_STROKE_LINEJOIN_ROUND:
		tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_ROUND));
		break;
	case SP_STROKE_LINEJOIN_BEVEL:
		tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_BEVEL));
		break;
	default:
		tb = NULL;
		break;
	}
	sp_stroke_style_set_join_buttons (spw, tb);

	switch (style->stroke_linecap.value) {
	case SP_STROKE_LINECAP_BUTT:
		tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_BUTT));
		break;
	case SP_STROKE_LINECAP_ROUND:
		tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_ROUND));
		break;
	case SP_STROKE_LINECAP_SQUARE:
		tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_SQUARE));
		break;
	default:
		tb = NULL;
		break;
	}
	sp_stroke_style_set_cap_buttons (spw, tb);

#ifdef MARKERS
	/* Toggle buttons for markers - marker-start, marker-mid, and marker-end */
	/* TODO:  There's also a generic 'marker' that applies to all, but we'll leave that for later */
	tb = (GtkWidget*)gtk_object_get_data (GTK_OBJECT (spw), marker_id_to_string(style->marker_start.value));
	sp_stroke_style_set_marker_buttons (spw, tb);
	  
	tb = (GtkWidget*)gtk_object_get_data (GTK_OBJECT (spw), marker_id_to_string(style->marker_mid.value));
	sp_stroke_style_set_marker_buttons (spw, tb);
	
	tb = (GtkWidget*)gtk_object_get_data (GTK_OBJECT (spw), marker_id_to_string(style->marker_end.value));
	sp_stroke_style_set_marker_buttons (spw, tb);
#endif
	/* Dash */
	if (style->stroke_dash.n_dash > 0) {
		double d[64];
		int len, i;
		len = MIN (style->stroke_dash.n_dash, 64);
		for (i = 0; i < len; i++) {
			d[i] = style->stroke_dash.dash[i] / style->stroke_width.computed;
		}
		sp_dash_selector_set_dash (SP_DASH_SELECTOR (dsel), len, d, style->stroke_dash.offset / style->stroke_width.computed);
	} else {
		sp_dash_selector_set_dash (SP_DASH_SELECTOR (dsel), 0, NULL, 0.0);
	}

	gtk_widget_set_sensitive (sset, TRUE);

	sp_style_unref (style);

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

static void
sp_stroke_style_set_scaled_dash (SPCSSAttr *css, int ndash, double *dash, double offset, double scale)
{
	if (ndash > 0) {
		gchar c[1024];
		int i, pos;
		pos = 0;
		for (i = 0; i < ndash; i++) {
			pos += g_snprintf (c + pos, 1022 - pos, "%g", dash[i] * scale);
			if ((i < (ndash - 1)) && (pos < 1020)) {
				c[pos] = ',';
				pos += 1;
			}
		}
		c[pos] = 0;
		sp_repr_css_set_property (css, "stroke-dasharray", c);
		g_snprintf (c, 1024, "%g", offset * scale);
		sp_repr_css_set_property (css, "stroke-dashoffset", c);
	} else {
		sp_repr_css_set_property (css, "stroke-dasharray", "none");
		sp_repr_css_set_property (css, "stroke-dashoffset", NULL);
	}
}

static void
sp_stroke_style_scale_line (SPWidget *spw)
{
	GtkAdjustment *wadj;
	SPUnitSelector *us;
	SPDashSelector *dsel;
	const GSList *items, *i, *r;
	GSList *reprs;
	SPCSSAttr *css;
	gchar c[32];

	wadj = GTK_ADJUSTMENT(gtk_object_get_data (GTK_OBJECT (spw), "width"));
	us = SP_UNIT_SELECTOR(gtk_object_get_data (GTK_OBJECT (spw), "units"));
	dsel = SP_DASH_SELECTOR(gtk_object_get_data (GTK_OBJECT (spw), "dash"));

	if (spw->inkscape) {
		/* fixme: */
		if (!SP_WIDGET_DOCUMENT (spw)) return;
		reprs = NULL;
		items = sp_widget_get_item_list (spw);
		for (i = items; i != NULL; i = i->next) {
			reprs = g_slist_prepend (reprs, SP_OBJECT_REPR (i->data));
		}
	} else {
		reprs = g_slist_prepend (NULL, spw->repr);
		items = NULL;
	}

	/* fixme: Create some standardized method */
	css = sp_repr_css_attr_new ();

	if (items) {
		for (i = items; i != NULL; i = i->next) {
			NRMatrixF i2d, d2i;
			double length, dist;
			double *dash, offset;
			int ndash;
			length = wadj->value;
			sp_dash_selector_get_dash (dsel, &ndash, &dash, &offset);
			/* Set stroke width */
			sp_convert_distance (&length, sp_unit_selector_get_unit (us), SP_PS_UNIT);
			sp_item_i2d_affine (SP_ITEM (i->data), &i2d);
			nr_matrix_f_invert (&d2i, &i2d);
			dist = length * NR_MATRIX_DF_EXPANSION (&d2i);
			g_snprintf (c, 32, "%g", dist);
			sp_repr_css_set_property (css, "stroke-width", c);
			/* Set dash */
			sp_stroke_style_set_scaled_dash (css, ndash, dash, offset, dist);
			sp_repr_css_change_recursive (SP_OBJECT_REPR (i->data), css, "style");
			g_free (dash);
		}
	} else {
		for (r = reprs; r != NULL; r = r->next) {
			double length;
			double *dash, offset;
			int ndash;
			length = wadj->value;
			sp_dash_selector_get_dash (dsel, &ndash, &dash, &offset);
			sp_convert_distance (&length, sp_unit_selector_get_unit (us), SP_PS_UNIT);
			g_snprintf (c, 32, "%g", length * 1.25);
			sp_repr_css_set_property (css, "stroke-width", c);
			sp_stroke_style_set_scaled_dash (css, ndash, dash, offset, length);
			sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
			g_free (dash);
		}
	}

	sp_repr_css_attr_unref (css);
	if (spw->inkscape) sp_document_done (SP_WIDGET_DOCUMENT (spw));

	g_slist_free (reprs);
}

static void
sp_stroke_style_width_changed (GtkAdjustment *adj, SPWidget *spw)
{
	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	sp_stroke_style_scale_line (spw);
}

static void
sp_stroke_style_line_dash_changed (SPDashSelector *dsel, SPWidget *spw)
{
	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	sp_stroke_style_scale_line (spw);
}

static void
sp_stroke_style_any_toggled (GtkToggleButton *tb, SPWidget *spw)
{
	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	if (gtk_toggle_button_get_active (tb)) {
		const GSList *items, *i, *r;
		GSList *reprs;
		const gchar *join, *cap, *start_marker;
		SPCSSAttr *css;

		items = sp_widget_get_item_list (spw);
		join = (const gchar *)gtk_object_get_data (GTK_OBJECT (tb), "join");
		cap = (const gchar *)gtk_object_get_data (GTK_OBJECT (tb), "cap");
		start_marker = (const gchar*)gtk_object_get_data (GTK_OBJECT (tb), "start_marker");

		if (spw->inkscape) {
			reprs = NULL;
			items = sp_widget_get_item_list (spw);
			for (i = items; i != NULL; i = i->next) {
				reprs = g_slist_prepend (reprs, SP_OBJECT_REPR (i->data));
			}
		} else {
			reprs = g_slist_prepend (NULL, spw->repr);
			items = NULL;
		}

		/* fixme: Create some standardized method */
		css = sp_repr_css_attr_new ();

		if (join) {
			sp_repr_css_set_property (css, "stroke-linejoin", join);
			for (r = reprs; r != NULL; r = r->next) {
				sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
			}
			sp_stroke_style_set_join_buttons (spw, GTK_WIDGET (tb));
		} else if (cap) {
			sp_repr_css_set_property (css, "stroke-linecap", cap);
			for (r = reprs; r != NULL; r = r->next) {
				sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
			}
			sp_stroke_style_set_cap_buttons (spw, GTK_WIDGET (tb));
		} else {
		        sp_repr_css_set_property (css, "stroke-start-marker", start_marker);
			for (r = reprs; r != NULL; r = r->next) {
			        sp_repr_css_change_recursive ((SPRepr *) r->data, css, "style");
			}
			sp_stroke_style_set_cap_buttons (spw, GTK_WIDGET (tb));
		}

		sp_repr_css_attr_unref (css);
		if (spw->inkscape) sp_document_done (SP_WIDGET_DOCUMENT (spw));

		g_slist_free (reprs);
	}
}

/* Helpers */

static void
sp_stroke_style_get_average_color_rgba (const GSList *objects, gfloat *c)
{
	gint num;

	c[0] = 0.0;
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
	num = 0;

	while (objects) {
		SPObject *object;
		gfloat d[3];
		object = SP_OBJECT (objects->data);
		if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
			sp_color_get_rgb_floatv (&object->style->stroke.value.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += SP_SCALE24_TO_FLOAT (object->style->stroke_opacity.value);
		}
		num += 1;
		objects = objects->next;
	}

	c[0] /= num;
	c[1] /= num;
	c[2] /= num;
	c[3] /= num;
}

static void
sp_stroke_style_get_average_color_cmyka (const GSList *objects, gfloat *c)
{
	gint num;

	c[0] = 0.0;
	c[1] = 0.0;
	c[2] = 0.0;
	c[3] = 0.0;
	c[4] = 0.0;
	num = 0;

	while (objects) {
		SPObject *object;
		gfloat d[4];
		object = SP_OBJECT (objects->data);
		if (object->style->stroke.type == SP_PAINT_TYPE_COLOR) {
			sp_color_get_cmyk_floatv (&object->style->stroke.value.color, d);
			c[0] += d[0];
			c[1] += d[1];
			c[2] += d[2];
			c[3] += d[3];
			c[4] += SP_SCALE24_TO_FLOAT (object->style->stroke_opacity.value);
		}
		num += 1;
		objects = objects->next;
	}

	c[0] /= num;
	c[1] /= num;
	c[2] /= num;
	c[3] /= num;
	c[4] /= num;
}

static SPPaintSelectorMode
sp_stroke_style_determine_paint_selector_mode (SPStyle *style)
{
	SPColorSpaceType cstype;

	switch (style->stroke.type) {
	case SP_PAINT_TYPE_NONE:
		return SP_PAINT_SELECTOR_MODE_NONE;
	case SP_PAINT_TYPE_COLOR:
		cstype = sp_color_get_colorspace_type (&style->stroke.value.color);
		switch (cstype) {
		case SP_COLORSPACE_TYPE_RGB:
			return SP_PAINT_SELECTOR_MODE_COLOR_RGB;
		case SP_COLORSPACE_TYPE_CMYK:
			return SP_PAINT_SELECTOR_MODE_COLOR_CMYK;
		default:
			g_warning ("file %s: line %d: Unknown colorspace type %d", __FILE__, __LINE__, cstype);
			return SP_PAINT_SELECTOR_MODE_NONE;
		}
	case SP_PAINT_TYPE_PAINTSERVER:
		if (SP_IS_LINEARGRADIENT (SP_STYLE_STROKE_SERVER (style))) {
			return SP_PAINT_SELECTOR_MODE_GRADIENT_LINEAR;
		} else if (SP_IS_RADIALGRADIENT (SP_STYLE_STROKE_SERVER (style))) {
			return SP_PAINT_SELECTOR_MODE_GRADIENT_RADIAL;
		}
		return SP_PAINT_SELECTOR_MODE_NONE;
	default:
		g_warning ("file %s: line %d: Unknown paint type %d", __FILE__, __LINE__, style->stroke.type);
		break;
	}

	return SP_PAINT_SELECTOR_MODE_NONE;
}

static void
sp_stroke_style_set_join_buttons (SPWidget *spw, GtkWidget *active)
{
	GtkWidget *tb;

	tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_MITER));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_ROUND));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_JOIN_BEVEL));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
}

static void
sp_stroke_style_set_cap_buttons (SPWidget *spw, GtkWidget *active)
{
	GtkWidget *tb;

	tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_BUTT));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_ROUND));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
	tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_CAP_SQUARE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
}

/**
 * Creates a set of marker buttons.  This routine creates togglebuttons for the
 * line markers.  Currently it provides just three options - none, filled or
 * hollow arrows.  This is intended only as a quicky way to get proof of concept
 * arrowhead functionality and is intended to be replaced by a more powerful and
 * flexible system later on.
 * 
 * spw - the widget to put the buttons onto.
 * active - the currently selected button.
 */
static void
sp_stroke_style_set_marker_buttons (SPWidget *spw, GtkWidget *active)
{
  /* A toggle button */
  GtkWidget *tb;
  
  tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_START_NONE));
  g_assert(tb != NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));

  tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_START_FILLED_ARROW));
  g_assert(tb != NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));

  tb = GTK_WIDGET(gtk_object_get_data (GTK_OBJECT (spw), INKSCAPE_STOCK_START_HOLLOW_ARROW));
  g_assert(tb != NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tb), (active == tb));
}

