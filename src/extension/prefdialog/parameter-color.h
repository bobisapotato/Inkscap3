// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEEN_INK_EXTENSION_PARAMCOLOR_H__
#define SEEN_INK_EXTENSION_PARAMCOLOR_H__
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "parameter.h"
#include "ui/selected-color.h"

class SPDocument;

namespace Gtk {
class Widget;
}

namespace Inkscape {
namespace XML {
class Node;
}

namespace Extension {

class ParamColor : public InxParameter {
private:
    void _onColorChanged();

    Inkscape::UI::SelectedColor _color;
    sigc::connection _color_changed;
    sigc::connection _color_released;

public:
    ParamColor(Inkscape::XML::Node *xml, Inkscape::Extension::Extension *ext);
    ~ParamColor() override;

    /** Returns \c _value, with a \i const to protect it. */
    unsigned int get(SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/ ) const { return _color.value(); }

    unsigned int set(unsigned int in, SPDocument *doc, Inkscape::XML::Node *node);

    Gtk::Widget *get_widget(SPDocument *doc, Inkscape::XML::Node *node, sigc::signal<void> *changeSignal) override;

    std::string value_to_string() const override;

    sigc::signal<void> *_changeSignal;

}; // class ParamColor

}  // namespace Extension
}  // namespace Inkscape

#endif // SEEN_INK_EXTENSION_PARAMCOLOR_H__

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
