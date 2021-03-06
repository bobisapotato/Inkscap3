#define __OBJECT_PROPERTIES_C__

/*
 * Shape and tool style dialog
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <config.h>

#include <glib.h>
#include <libart_lgpl/art_affine.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtksignal.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkiconfactory.h>

#include "helper/sp-intl.h"
#include "helper/window.h"
#include "widgets/sp-widget.h"
#include "macros.h"
#include "inkscape.h"
#include "fill-style.h"
#include "stroke-style.h"
#include "dialog-events.h"
#include "verbs.h"
#include "interface.h"
#include "object-properties.h"
#include "inkscape-stock.h"
#include "prefs-utils.h"

static GtkWidget *dlg = NULL;
static win_data wd;
static gint x = -1000, y = -1000, w = 0, h = 0; // impossible original values to make sure they are read from prefs
static gchar *prefs_path = "dialogs.fillstroke";

static void
sp_object_properties_dialog_destroy (GtkObject *object, gpointer data)
{
	sp_signal_disconnect_by_data (INKSCAPE, dlg);
	wd.win = dlg = NULL;
	wd.stop = 0;
}

static gboolean
sp_object_properties_dialog_delete (GtkObject *object, GdkEvent *event, gpointer data)
{
	gtk_window_get_position ((GtkWindow *) dlg, &x, &y);
	gtk_window_get_size ((GtkWindow *) dlg, &w, &h);

	prefs_set_int_attribute (prefs_path, "x", x);
	prefs_set_int_attribute (prefs_path, "y", y);
	prefs_set_int_attribute (prefs_path, "w", w);
	prefs_set_int_attribute (prefs_path, "h", h);

	return FALSE; // which means, go ahead and destroy it
}

static void
sp_object_properties_style_activate (GtkMenuItem *menuitem, const gchar *key)
{
	GtkWidget *fs, *sp, *sl;

	fs = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (dlg), "fill");
	sp = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (dlg), "stroke-paint");
	sl = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (dlg), "stroke-line");

	if (key) {
		SPRepr *repr;
		repr = inkscape_get_repr (INKSCAPE, key);
		if (repr) {
			sp_widget_construct_repr (SP_WIDGET (fs), repr);
			sp_widget_construct_repr (SP_WIDGET (sp), repr);
			sp_widget_construct_repr (SP_WIDGET (sl), repr);
			gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
			gtk_widget_set_sensitive (GTK_WIDGET (sp), TRUE);
			gtk_widget_set_sensitive (GTK_WIDGET (sl), TRUE);
		} else {
			/* Big trouble */
			gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);
			gtk_widget_set_sensitive (GTK_WIDGET (sp), FALSE);
			gtk_widget_set_sensitive (GTK_WIDGET (sl), FALSE);
		}
	} else {
		sp_widget_construct_global (SP_WIDGET (fs), INKSCAPE);
		sp_widget_construct_global (SP_WIDGET (sp), INKSCAPE);
		sp_widget_construct_global (SP_WIDGET (sl), INKSCAPE);
		gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (sp), TRUE);
		gtk_widget_set_sensitive (GTK_WIDGET (sl), TRUE);
	}
}

int
sp_object_properties_page(GtkWidget *nb, GtkWidget *page,
			  char *label, char *dlg_name, char *label_image)
{
  GtkWidget *hb, *l, *px;
  
  hb = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hb);
  l = gtk_label_new (_(label));
  gtk_widget_show (l);
  gtk_box_pack_start (GTK_BOX (hb), l, FALSE, FALSE, 0);
  px = gtk_image_new_from_file (label_image);
  gtk_widget_show (px);
  gtk_box_pack_start (GTK_BOX (hb), px, FALSE, FALSE, 0);
  gtk_widget_show (page);
  gtk_notebook_append_page (GTK_NOTEBOOK (nb), page, hb);
  gtk_object_set_data (GTK_OBJECT (dlg), dlg_name, page);
  return 0;
}


static void
sp_object_properties_color_set (Inkscape *inkscape, SPColor *color, double opacity, GObject *dlg)
{
	GtkNotebook *nb;
	int pnum;

	nb = (GtkNotebook *)g_object_get_data (dlg, "notebook");
	pnum = gtk_notebook_get_current_page (nb);

	if (pnum == 0) {
		GtkWidget *fs;
		fs = (GtkWidget *)g_object_get_data (dlg, "fill");
		sp_fill_style_widget_system_color_set (fs, color, opacity);
	} else if (pnum == 1) {
		GtkWidget *ss;
		ss = (GtkWidget *)g_object_get_data (dlg, "stroke-paint");
		sp_stroke_style_paint_system_color_set (ss, color, opacity);
	}
}

void
sp_object_properties_dialog (void)
{
	if (!dlg) {
		GtkWidget *vb, *nb, *hb, *l, *page, *hs, *om, *m, *mi;

		gchar title[500];
		sp_ui_dialog_title_string (SP_VERB_DIALOG_FILL_STROKE, title);

		dlg = sp_window_new (title, TRUE);
		if (x == -1000 || y == -1000) {
			x = prefs_get_int_attribute (prefs_path, "x", 0);
			y = prefs_get_int_attribute (prefs_path, "y", 0);
		}
		if (w ==0 || h == 0) {
			w = prefs_get_int_attribute (prefs_path, "w", 0);
			h = prefs_get_int_attribute (prefs_path, "h", 0);
		}
		if (x != 0 || y != 0) 
			gtk_window_move ((GtkWindow *) dlg, x, y);
		else
			gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
		if (w && h) gtk_window_resize ((GtkWindow *) dlg, w, h);
		sp_transientize (dlg);
		wd.win = dlg;
		wd.stop = 0;
		g_signal_connect (G_OBJECT (INKSCAPE), "activate_desktop", G_CALLBACK (sp_transientize_callback), &wd);
		gtk_signal_connect (GTK_OBJECT (dlg), "event", GTK_SIGNAL_FUNC (sp_dialog_event_handler), dlg);

		gtk_signal_connect (GTK_OBJECT (dlg), "destroy", G_CALLBACK (sp_object_properties_dialog_destroy), dlg);
		gtk_signal_connect (GTK_OBJECT (dlg), "delete_event", G_CALLBACK (sp_object_properties_dialog_delete), dlg);
		g_signal_connect (G_OBJECT (INKSCAPE), "shut_down", G_CALLBACK (sp_object_properties_dialog_delete), dlg);

		vb = gtk_vbox_new (FALSE, 0);
		gtk_widget_show (vb);
		gtk_container_add (GTK_CONTAINER (dlg), vb); 

		nb = gtk_notebook_new ();
		gtk_widget_show (nb);
		gtk_box_pack_start (GTK_BOX (vb), nb, TRUE, TRUE, 0);
		gtk_object_set_data (GTK_OBJECT (dlg), "notebook", nb);

		/* Fill page */
		page = sp_fill_style_widget_new ();
		sp_object_properties_page(nb, page, "Fill", "fill",
					  INKSCAPE_GLADEDIR "/properties_fill.xpm");
 
		/* Stroke paint page */
		page = sp_stroke_style_paint_widget_new ();
		sp_object_properties_page(nb, page, "Stroke paint", "stroke-paint",
					  INKSCAPE_GLADEDIR "/properties_stroke.xpm");
 
		/* Stroke line page */
		page = sp_stroke_style_line_widget_new ();
		sp_object_properties_page(nb, page, "Stroke style", "stroke-line",
					  INKSCAPE_GLADEDIR "/properties_stroke.xpm");
 
		/* Modify style selector */
		hs = gtk_hseparator_new ();
		gtk_widget_show (hs);
		gtk_box_pack_start (GTK_BOX (vb), hs, FALSE, FALSE, 0);

		hb = gtk_hbox_new (FALSE, 4);
		gtk_widget_show (hb);
		gtk_box_pack_start (GTK_BOX (vb), hb, FALSE, FALSE, 0);

		l = gtk_label_new (_("Apply to:"));
		gtk_widget_show (l);
		gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hb), l, TRUE, TRUE, 4);
		om = gtk_option_menu_new ();
		gtk_widget_show (om);
		gtk_box_pack_start (GTK_BOX (hb), om, TRUE, TRUE, 0);

		m = gtk_menu_new ();

		mi = gtk_menu_item_new_with_label (_("Selected objects"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), NULL);
		mi = gtk_menu_item_new_with_label (_("All shape tools"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.shapes");
		mi = gtk_menu_item_new_with_label (_("Rectangle tool"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.shapes.rect");
		mi = gtk_menu_item_new_with_label (_("Arc tool"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.shapes.arc");
		mi = gtk_menu_item_new_with_label (_("Star tool"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.shapes.star");
		mi = gtk_menu_item_new_with_label (_("Spiral tool"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.shapes.spiral");
		mi = gtk_menu_item_new_with_label (_("Freehand and pen"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.freehand");
		mi = gtk_menu_item_new_with_label (_("Calligraphic line"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.calligraphic");
		mi = gtk_menu_item_new_with_label (_("Text"));
		gtk_menu_append (GTK_MENU (m), mi);
		gtk_signal_connect (GTK_OBJECT (mi), "activate", GTK_SIGNAL_FUNC (sp_object_properties_style_activate), (void *)"tools.text");

		gtk_widget_show_all (m);

		gtk_option_menu_set_menu (GTK_OPTION_MENU (om), m);

		g_signal_connect (G_OBJECT (INKSCAPE), "color_set", G_CALLBACK (sp_object_properties_color_set), dlg);

		gtk_widget_show (dlg);
	} else {
		gtk_window_present (GTK_WINDOW (dlg));
	}
}

void sp_object_properties_stroke (void)
{
	GtkWidget *nb;

	sp_object_properties_dialog ();

	nb = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (dlg), "notebook");

	gtk_notebook_set_page (GTK_NOTEBOOK (nb), 1);
}

void sp_object_properties_fill (void)
{
	GtkWidget *nb;

	sp_object_properties_dialog ();

	nb = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (dlg), "notebook");

	gtk_notebook_set_page (GTK_NOTEBOOK (nb), 0);
}

/*
 * Layout dialog
 */

#include <math.h>

#include <gtk/gtkframe.h>
#include <gtk/gtktable.h>
#include <gtk/gtkspinbutton.h>

#include "../helper/unit-menu.h"
#include "../widgets/sp-widget.h"
#include "../inkscape.h"
#include "../document.h"
#include "../desktop-handles.h"
#include "../selection.h"
#include "../selection-chemistry.h"
#include "../sp-object.h"

GtkWidget *sp_selection_layout_widget_new (void);
static void sp_selection_layout_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data);
static void sp_selection_layout_widget_change_selection (SPWidget *spw, SPSelection *selection, gpointer data);
static void sp_selection_layout_widget_update (SPWidget *spw, SPSelection *sel);
static void sp_object_layout_any_value_changed (GtkAdjustment *adj, SPWidget *spw);

static void
sp_object_layout_dialog_destroy (GtkObject *object, GtkWidget **reference)
{
	*reference = NULL;
}

void
sp_object_properties_layout (void)
{
	static GtkWidget *dialog = NULL;

	if (!dialog) {
		GtkWidget *w;
		dialog = sp_window_new (_("Object Size and Position"), TRUE);
		gtk_signal_connect (GTK_OBJECT (dialog), "destroy", GTK_SIGNAL_FUNC (sp_object_layout_dialog_destroy), &dialog);
		w = sp_selection_layout_widget_new ();
		gtk_widget_show (w);
		gtk_container_add (GTK_CONTAINER (dialog), w);
	}

	gtk_widget_show (dialog);
}

GtkWidget *
sp_selection_layout_widget_new (void)
{
	GtkWidget *spw, *vb, *f, *t, *l, *us, *px, *sb;
	GtkObject *a;

	spw = sp_widget_new_global (INKSCAPE);

	vb = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vb);
	gtk_container_add (GTK_CONTAINER (spw), vb);

	f = gtk_frame_new (_("Size and Position"));
	gtk_widget_show (f);
	gtk_container_set_border_width (GTK_CONTAINER (f), 4);
	gtk_box_pack_start (GTK_BOX (vb), f, FALSE, FALSE, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "frame", f);

	t = gtk_table_new (5, 3, FALSE);
	gtk_widget_show (t);
	gtk_container_set_border_width (GTK_CONTAINER (t), 4);
	gtk_table_set_row_spacings (GTK_TABLE (t), 4);
	gtk_table_set_col_spacings (GTK_TABLE (t), 4);
	gtk_container_add (GTK_CONTAINER (f), t);

	l = gtk_label_new (_("Units:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 0, 2, 0, 1, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	us = sp_unit_selector_new (SP_UNIT_ABSOLUTE);
	gtk_widget_show (us);
	gtk_table_attach (GTK_TABLE (t), us, 2, 3, 0, 1, (GtkAttachOptions)( GTK_EXPAND | GTK_FILL ), (GtkAttachOptions)0, 0, 0);
	gtk_object_set_data (GTK_OBJECT (spw), "units", us);

	px = gtk_image_new_from_stock (INKSCAPE_STOCK_ARROWS_HOR, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (px);
	gtk_table_attach (GTK_TABLE (t), px, 0, 1, 1, 2, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	l = gtk_label_new (_("X:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 1, 2, 1, 2, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 0.1, 10.0, 10.0);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	gtk_object_set_data (GTK_OBJECT (spw), "X", a);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 2, 3, 1, 2, (GtkAttachOptions)( GTK_EXPAND | GTK_FILL ), (GtkAttachOptions)0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_object_layout_any_value_changed), spw);

	px = gtk_image_new_from_stock (INKSCAPE_STOCK_ARROWS_VER, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (px);
	gtk_table_attach (GTK_TABLE (t), px, 0, 1, 2, 3, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	l = gtk_label_new (_("Y:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 1, 2, 2, 3, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 0.1, 10.0, 10.0);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	gtk_object_set_data (GTK_OBJECT (spw), "Y", a);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 2, 3, 2, 3, (GtkAttachOptions)( GTK_EXPAND | GTK_FILL ), (GtkAttachOptions)0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_object_layout_any_value_changed), spw);

	px = gtk_image_new_from_stock (INKSCAPE_STOCK_DIMENSION_HOR, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (px);
	gtk_table_attach (GTK_TABLE (t), px, 0, 1, 3, 4, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	l = gtk_label_new (_("Width:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 1, 2, 3, 4, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 0.1, 10.0, 10.0);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	gtk_object_set_data (GTK_OBJECT (spw), "width", a);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 2, 3, 3, 4, (GtkAttachOptions)( GTK_EXPAND | GTK_FILL ), (GtkAttachOptions)0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_object_layout_any_value_changed), spw);

	px = gtk_image_new_from_stock (INKSCAPE_STOCK_DIMENSION_VER, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_widget_show (px);
	gtk_table_attach (GTK_TABLE (t), px, 0, 1, 4, 5, (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
	l = gtk_label_new (_("Height:"));
	gtk_widget_show (l);
	gtk_misc_set_alignment (GTK_MISC (l), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (t), l, 1, 2, 4, 5, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	a = gtk_adjustment_new (0.0, -1e6, 1e6, 0.1, 10.0, 10.0);
	sp_unit_selector_add_adjustment (SP_UNIT_SELECTOR (us), GTK_ADJUSTMENT (a));
	gtk_object_set_data (GTK_OBJECT (spw), "height", a);
	sb = gtk_spin_button_new (GTK_ADJUSTMENT (a), 0.1, 2);
	gtk_widget_show (sb);
	gtk_table_attach (GTK_TABLE (t), sb, 2, 3, 4, 5, (GtkAttachOptions)( GTK_EXPAND | GTK_FILL ), (GtkAttachOptions)0, 0, 0);
	gtk_signal_connect (GTK_OBJECT (a), "value_changed", GTK_SIGNAL_FUNC (sp_object_layout_any_value_changed), spw);

	gtk_signal_connect (GTK_OBJECT (spw), "modify_selection", GTK_SIGNAL_FUNC (sp_selection_layout_widget_modify_selection), NULL);
	gtk_signal_connect (GTK_OBJECT (spw), "change_selection", GTK_SIGNAL_FUNC (sp_selection_layout_widget_change_selection), NULL);

	sp_selection_layout_widget_update (SP_WIDGET (spw), SP_ACTIVE_DESKTOP ? SP_DT_SELECTION (SP_ACTIVE_DESKTOP) : NULL);

	return spw;
}

static void
sp_selection_layout_widget_modify_selection (SPWidget *spw, SPSelection *selection, guint flags, gpointer data)
{
	if (flags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_PARENT_MODIFIED_FLAG)) {
		sp_selection_layout_widget_update (spw, selection);
	}
}

static void
sp_selection_layout_widget_change_selection (SPWidget *spw, SPSelection *selection, gpointer data)
{
	sp_selection_layout_widget_update (spw, selection);
}

static void
sp_selection_layout_widget_update (SPWidget *spw, SPSelection *sel)
{
	GtkWidget *f;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	f = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (spw), "frame");

	if (sel && !sp_selection_is_empty (sel)) {
		NRRectF bbox;

		sp_selection_bbox (sel, &bbox);

		if ((bbox.x1 - bbox.x0 > 1e-6) && (bbox.y1 - bbox.y0 > 1e-6)) {
			GtkWidget *us;
			GtkAdjustment *a;
			const SPUnit *unit;

			us = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (spw), "units");
			unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (us));

			a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "X");
			gtk_adjustment_set_value (a, sp_points_get_units (bbox.x0, unit));
			a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "Y");
			gtk_adjustment_set_value (a, sp_points_get_units (bbox.y0, unit));
			a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "width");
			gtk_adjustment_set_value (a, sp_points_get_units (bbox.x1 - bbox.x0, unit));
			a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "height");
			gtk_adjustment_set_value (a, sp_points_get_units (bbox.y1 - bbox.y0, unit));

			gtk_widget_set_sensitive (f, TRUE);
		} else {
			gtk_widget_set_sensitive (f, FALSE);
		}
	} else {
		gtk_widget_set_sensitive (f, FALSE);
	}

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

static void
sp_object_layout_any_value_changed (GtkAdjustment *adj, SPWidget *spw)
{
	GtkWidget *us;
	GtkAdjustment *a;
	const SPUnit *unit;
	NRRectF bbox;
	gdouble x0, y0, x1, y1;
	SPSelection *sel;

	if (gtk_object_get_data (GTK_OBJECT (spw), "update")) return;

	sel = SP_WIDGET_SELECTION (spw);
	us = (GtkWidget *)gtk_object_get_data (GTK_OBJECT (spw), "units");
	unit = sp_unit_selector_get_unit (SP_UNIT_SELECTOR (us));
	if (sp_unit_selector_update_test (SP_UNIT_SELECTOR (us))) {
		/*
		 * When only units are being changed, don't treat changes
		 * to adjuster values as object changes.
		 */
		return;
	}
	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (TRUE));

	sp_selection_bbox (sel, &bbox);
	g_return_if_fail (bbox.x1 - bbox.x0 > 1e-9);
	g_return_if_fail (bbox.y1 - bbox.y0 > 1e-9);

	a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "X");
	x0 = sp_units_get_points (a->value, unit);
	a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "Y");
	y0 = sp_units_get_points (a->value, unit);
	a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "width");
	x1 = x0 + sp_units_get_points (a->value, unit);
	a = (GtkAdjustment *)gtk_object_get_data (GTK_OBJECT (spw), "height");
	y1 = y0 + sp_units_get_points (a->value, unit);

	if ((fabs (x0 - bbox.x0) > 1e-6) || (fabs (y0 - bbox.y0) > 1e-6) || (fabs (x1 - bbox.x1) > 1e-6) || (fabs (y1 - bbox.y1) > 1e-6)) {
		gdouble p2o[6], o2n[6], scale[6], s[6], t[6];

		art_affine_translate (p2o, -bbox.x0, -bbox.y0);
		art_affine_scale (scale, (x1 - x0) / (bbox.x1 - bbox.x0), (y1 - y0) / (bbox.y1 - bbox.y0));
		art_affine_translate (o2n, x0, y0);
		art_affine_multiply (s , p2o, scale);
		art_affine_multiply (t , s, o2n);
		sp_selection_apply_affine (sel, t);
#if 1
		sp_document_done (SP_WIDGET_DOCUMENT (spw));
#endif
	}

	gtk_object_set_data (GTK_OBJECT (spw), "update", GINT_TO_POINTER (FALSE));
}

