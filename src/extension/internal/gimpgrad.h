/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2004-2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glibmm/ustring.h>

#include <extension/implementation/implementation.h>
#include <extension/extension-forward.h>

namespace Inkscape {
namespace Extension {
namespace Internal {

/** \brief  Implementation class of the GIMP gradient plugin.  This mostly
            just creates a namespace for the GIMP gradient plugin today.
*/
class GimpGrad : public Inkscape::Extension::Implementation::Implementation {
private:
    Glib::ustring GimpGrad::new_stop (ColorRGBA in_color, float location);

public:
    bool load(Inkscape::Extension::Extension *module);
    void unload(Inkscape::Extension::Extension *module);
    SPDocument *open(Inkscape::Extension::Input *module, gchar const *filename);

	static void init (void);
};


} } }  /* namespace Internal; Extension; Inkscape */

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
