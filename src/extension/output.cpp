/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "implementation/implementation.h"
#include "output.h"

/* Inkscape::Extension::Output */

namespace Inkscape {
namespace Extension {

/**
    \return   None
    \brief    Builds a SPModuleOutput object from a XML description
    \param    module  The module to be initialized
    \param    repr    The XML description in a SPRepr tree

    Okay, so you want to build a SPModuleOutput object.

    This function first takes and does the build of the parent class,
    which is SPModule.  Then, it looks for the <output> section of the
    XML description.  Under there should be several fields which
    describe the output module to excruciating detail.  Those are parsed,
    copied, and put into the structure that is passed in as module.
    Overall, there are many levels of indentation, just to handle the
    levels of indentation in the XML file.
*/
Output::Output (SPRepr * in_repr, Implementation::Implementation * in_imp) : Extension(in_repr, in_imp)
{
    mimetype = NULL;
    extension = NULL;
    filetypename = NULL;
    filetypetooltip = NULL;
	dataloss = TRUE;

    if (repr != NULL) {
        SPRepr * child_repr;

        child_repr = sp_repr_children(repr);

        while (child_repr != NULL) {
            if (!strcmp(sp_repr_name(child_repr), "output")) {
                child_repr = sp_repr_children(child_repr);
                while (child_repr != NULL) {
                    if (!strcmp(sp_repr_name(child_repr), "extension")) {
                        g_free (extension);
                        extension = g_strdup(sp_repr_content(sp_repr_children(child_repr)));
                    }
                    if (!strcmp(sp_repr_name(child_repr), "mimetype")) {
                        g_free (mimetype);
                        mimetype = g_strdup(sp_repr_content(sp_repr_children(child_repr)));
                    }
                    if (!strcmp(sp_repr_name(child_repr), "filetypename")) {
                        g_free (filetypename);
                        filetypename = g_strdup(sp_repr_content(sp_repr_children(child_repr)));
                    }
                    if (!strcmp(sp_repr_name(child_repr), "filetypetooltip")) {
                        g_free (filetypetooltip);
                        filetypetooltip = g_strdup(sp_repr_content(sp_repr_children(child_repr)));
                    }
                    if (!strcmp(sp_repr_name(child_repr), "dataloss")) {
                        if (!strcmp(sp_repr_content(sp_repr_children(child_repr)), "FALSE")) {
							dataloss = FALSE;
						}
					}

                    child_repr = sp_repr_next(child_repr);
                }

                break;
            }

            child_repr = sp_repr_next(child_repr);
        }

    }
}

/**
    \brief  Destroy an output extension
*/
Output::~Output (void)
{
    g_free(mimetype);
    g_free(extension);
    g_free(filetypename);
    g_free(filetypetooltip);
    return;
}

/**
    \return  Whether this extension checks out
	\brief   Validate this extension

	This function checks to make sure that the output extension has
	a filename extension and a MIME type.  Then it calls the parent
	class' check function which also checks out the implmentation.
*/
bool
Output::check (void)
{
	if (extension == NULL)
		return FALSE;
	if (mimetype == NULL)
		return FALSE;

	return Extension::check();
}

/**
    \return  IETF mime-type for the extension
	\brief   Get the mime-type that describes this extension
*/
gchar *
Output::get_mimetype(void)
{
    return mimetype;
}

/**
    \return  Filename extension for the extension
	\brief   Get the filename extension for this extension
*/
gchar *
Output::get_extension(void)
{
    return extension;
}

/**
    \return  The name of the filetype supported
	\brief   Get the name of the filetype supported
*/
gchar *
Output::get_filetypename(void)
{
    return filetypename;
}

/**
    \return  Tooltip giving more information on the filetype
	\brief   Get the tooltip for more information on the filetype
*/
gchar *
Output::get_filetypetooltip(void)
{
    return filetypetooltip;
}

/**
    \return  A dialog to get settings for this extension
	\brief   Create a dialog for preference for this extension

	Calls the implementation to get the preferences.
*/
GtkDialog *
Output::prefs (void)
{
    return imp->prefs_output(this);
}

/**
    \return  None
	\brief   Save a document as a file
	\param   doc  Document to save
	\param   uri  File to save the document as

	This function does a little of the dirty work involved in saving
	a document so that the implementation only has to worry about geting
	bits on the disk.

	The big thing that it does is remove and readd the fields that are
	only used at runtime and shouldn't be saved.  One that may surprise
	people is the output extension.  This is not saved so that the IDs
	could be changed, and old files will still work properly.

	After the file is saved by the implmentation the output_extension
	and dataloss variables are recreated.  The output_extension is set
	to this extension so that future saves use this extension.  Dataloss
	is set so that a warning will occur on closing the document that
	there may be some dataloss from this extension.
*/
void
Output::save (SPDocument * doc, const gchar * uri)
{
	SPRepr * repr;

	repr = sp_document_repr_root(doc);

	sp_document_set_undo_sensitive (doc, FALSE);
	sp_repr_set_attr(repr, "inkscape:output_extension", NULL);
	sp_repr_set_attr(repr, "inkscape:dataloss", NULL);
	sp_repr_set_attr(repr, "sodipodi:modified", NULL);
	sp_document_set_undo_sensitive (doc, TRUE);

    imp->save(this, doc, uri);

	sp_document_set_undo_sensitive (doc, FALSE);
	sp_repr_set_attr(repr, "inkscape:output_extension", get_id());
	if (dataloss) {
		sp_repr_set_attr(repr, "inkscape:dataloss", "TRUE");
	}
	sp_document_set_undo_sensitive (doc, TRUE);

	return;
}

}; }; /* namespace Inkscape, Extension */

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
