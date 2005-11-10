/*
    Author:  Ted Gould <ted@gould.cx>
    Copyright (c) 2003-2005

    This code is licensed under the GNU GPL.  See COPYING for details.
 
    This file is the backend to the extensions system.  These are
    the parts of the system that most users will never see, but are
    important for implementing the extensions themselves.  This file
    contains the base class for all of that.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "../extension.h"
#include "implementation.h"
#include "libnr/nr-point.h"

#include <extension/output.h>
#include <extension/input.h>
#include <extension/effect.h>

namespace Inkscape {
namespace Extension {
namespace Implementation {

/**
	\return   Was the load sucessful?
	\brief    This function is the stub load.  It just returns sucess.
	\param    module   The Extension that should be loaded.
*/
bool 
Implementation::load (Inkscape::Extension::Extension * module) {
	return TRUE;
} /* Implementation::load */

void 
Implementation::unload (Inkscape::Extension::Extension * module) {
	return;
} /* Implementation::unload */

bool
Implementation::check (Inkscape::Extension::Extension * module) {
	/* If there are no checks, they all pass */
	return TRUE;
} /* Implemenation::check */

Gtk::Widget *
Implementation::prefs_input (Inkscape::Extension::Input * module, gchar const * filename) {
	return module->autogui();
} /* Implementation::prefs_input */

SPDocument *
Implementation::open (Inkscape::Extension::Input * module, gchar const * filename) {
    /* throw open_failed(); */
    return NULL;
} /* Implementation::open */

Gtk::Widget *
Implementation::prefs_output (Inkscape::Extension::Output * module) {
	return module->autogui();
} /* Implementation::prefs_output */

void
Implementation::save (Inkscape::Extension::Output * module, SPDocument * doc, gchar const * filename) {
	/* throw save_fail */
	return;
} /* Implementation::save */

Gtk::Widget *
Implementation::prefs_effect (Inkscape::Extension::Effect * module, Inkscape::UI::View::View * view) {
	return module->autogui();
} /* Implementation::prefs_effect */

void
Implementation::effect (Inkscape::Extension::Effect * module, Inkscape::UI::View::View * document) {
	/* throw filter_fail */
	return;
} /* Implementation::filter */

unsigned int
Implementation::setup (Inkscape::Extension::Print * module)
{
	return 0;
}

unsigned int
Implementation::set_preview (Inkscape::Extension::Print * module)
{
	return 0;
}


unsigned int
Implementation::begin (Inkscape::Extension::Print * module, SPDocument *doc)
{
	return 0;
}

unsigned int
Implementation::finish (Inkscape::Extension::Print * module)
{
	return 0;
}


/* Rendering methods */
unsigned int
Implementation::bind (Inkscape::Extension::Print * module, NRMatrix const *transform, float opacity)
{
	return 0;
}

unsigned int
Implementation::release (Inkscape::Extension::Print * module)
{
	return 0;
}

unsigned int
Implementation::comment (Inkscape::Extension::Print * module, char const * comment)
{
	return 0;
}

unsigned int
Implementation::fill (Inkscape::Extension::Print * module, NRBPath const *bpath, NRMatrix const *ctm, SPStyle const *style,
			   NRRect const *pbox, NRRect const *dbox, NRRect const *bbox)
{
	return 0;
}

unsigned int
Implementation::stroke (Inkscape::Extension::Print * module, NRBPath const *bpath, NRMatrix const *transform, SPStyle const *style,
			 NRRect const *pbox, NRRect const *dbox, NRRect const *bbox)
{
	return 0;
}

unsigned int
Implementation::image (Inkscape::Extension::Print * module, unsigned char *px, unsigned int w, unsigned int h, unsigned int rs,
			NRMatrix const *transform, SPStyle const *style)
{
	return 0;
}

unsigned int
Implementation::text (Inkscape::Extension::Print * module, char const *text,
                      NR::Point p, SPStyle const *style)
{
	return 0;
}

/**
   \brief  Tell the printing engine whether text should be text or path
   \retval TRUE  Render the text as a path
   \retval FALSE Render text using the text function (above)
 
    Default value is FALSE because most printing engines will support
    paths more than they'll support text.  (at least they do today)
*/
bool
Implementation::textToPath(Inkscape::Extension::Print * ext)
{
    return FALSE;
}


}  /* namespace Implementation */
}  /* namespace Extension */
}  /* namespace Inkscape */

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
