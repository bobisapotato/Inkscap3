#ifndef SEEN_ICON_PREVIEW_H
#define SEEN_ICON_PREVIEW_H
/*
 * A simple dialog for previewing icon representation.
 *
 * Authors:
 *   Jon A. Cruz
 *   Bob Jamison
 *   Other dudes from The Inkscape Organization
 *
 * Copyright (C) 2004,2005 The Inkscape Organization
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/toggletoolbutton.h>

#include "sp-object.h"
#include "ui/widget/panel.h"

namespace Inkscape {
namespace UI {
namespace Dialogs {


/**
 * A panel that displays an icon preview
 */
class IconPreviewPanel : public Inkscape::UI::Widget::Panel
{
public:
    IconPreviewPanel();
    //IconPreviewPanel(Glib::ustring const &label);

    static IconPreviewPanel& getInstance();

    void refreshPreview();

private:
    IconPreviewPanel(IconPreviewPanel const &); // no copy
    IconPreviewPanel &operator=(IconPreviewPanel const &); // no assign


    void on_button_clicked(int which);
    void renderPreview( SPObject* obj );
    void updateMagnify();

    static IconPreviewPanel* instance;

    Gtk::Tooltips   tips;

    Gtk::VBox       iconBox;
    Gtk::HPaned     splitter;

    int hot;
    int numEntries;
    int* sizes;

    Gtk::Button           *refreshButton;

    guchar** pixMem;
    Gtk::Image** images;
    Gtk::Image* magnified;
    Glib::ustring** labels;
    Gtk::ToggleToolButton** buttons;
};

} //namespace Dialogs
} //namespace UI
} //namespace Inkscape



#endif // SEEN_ICON_PREVIEW_H
