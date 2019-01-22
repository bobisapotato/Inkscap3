// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkscape - An SVG editor.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */


#include "inkscape-window.h"
#include "inkscape.h"   // SP_ACTIVE_DESKTOP
#include "enums.h"      // PREFS_WINDOW_GEOMETRY_NONE
#include "shortcuts.h"

#include "object/sp-namedview.h"  // TODO Remove need for this!

#include "ui/drag-and-drop.h"  // Move to canvas?
#include "ui/monitor.h" // get_monitor_geometry_at_point()

#include "ui/drag-and-drop.h"

#include "widgets/desktop-widget.h"

InkscapeWindow::InkscapeWindow(SPDocument* document)
    : _document(document)
{
    if (!_document) {
        std::cerr << "InkscapeWindow::InkscapeWindow: null document!" << std::endl;
        return;
    }

    // Register window with application.
    Glib::RefPtr<Gio::Application> gio_app = Gio::Application::get_default();
    Glib::RefPtr<Gtk::Application> app = Glib::RefPtr<Gtk::Application>::cast_dynamic(gio_app);
    if (app) {
        set_application(app);  // Same as Gtk::Application::add_window()
    } else {
        std::cerr << "InkscapeWindow::InkscapeWindow:: Didn't get app!" << std::endl;
    }

    set_resizable(true);

    sp_ui_drag_setup(this);

     // =============== Build interface ===============

    // Main box
    _mainbox = Gtk::manage(new Gtk::Box);
    _mainbox->set_name("DesktopMainBox");
    _mainbox->show();
    add(*_mainbox);

    // Menu bar

    // Desktop widget (=> MultiPaned)
    _desktop_widget = sp_desktop_widget_new(_document);
    _desktop_widget->window = this;
    gtk_container_add(GTK_CONTAINER(_mainbox->gobj()), GTK_WIDGET(_desktop_widget));
    gtk_widget_show(GTK_WIDGET(_desktop_widget));
    _desktop = _desktop_widget->desktop;

    // Pallet

    // Status bar

    // ================== Callbacks ==================
    signal_key_press_event().connect(   sigc::mem_fun(*this, &InkscapeWindow::key_press));
    signal_delete_event().connect(      sigc::mem_fun(*_desktop, &SPDesktop::onDeleteUI));
    signal_window_state_event().connect(sigc::mem_fun(*_desktop, &SPDesktop::onWindowStateEvent));
    signal_focus_in_event().connect(    sigc::mem_fun(*_desktop_widget, &SPDesktopWidget::onFocusInEvent));

    // =================== Actions ===================


    // ================ Window Options ==============

    show(); // Must show before resize!

    // Resize the window to match the document properties
    sp_namedview_window_from_document(_desktop); // This should probably be a member function here.

    sp_namedview_update_layers_from_document(_desktop);

}

bool
InkscapeWindow::key_press(GdkEventKey* event)
{
    unsigned shortcut = sp_shortcut_get_for_event(event);
    return sp_shortcut_invoke (shortcut, _desktop);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :