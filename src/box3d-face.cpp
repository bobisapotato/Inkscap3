#define __SP_3DBOX_FACE_C__

/*
 * Face of a 3D box ('perspectivic rectangle')
 *
 * Authors:
 *   Maximilian Albert <Anhalter42@gmx.de>
 *
 * Copyright (C) 2007  authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "svg/svg.h"
#include "box3d-face.h"
#include "prefs-utils.h"

// FIXME: It's quite redundant to pass the box plus the corners plus the axes. At least the corners can
//        theoretically be reconstructed from the box and the axes, but in order to do this we need
//        access to box->corners, which is not possible if we only have a forward declaration of SP3DBox
//        in box3d-face.h. (But we can't include box3d.h itself because the latter already includes
//        box3d-face.h).
Box3DFace::Box3DFace(SP3DBox *box, NR::Point &A, NR::Point &B, NR::Point &C, NR::Point &D,
                     Box3D::Axis plane, Box3D::FrontOrRear rel_pos)
    : front_or_rear (rel_pos), path (NULL), parent_box3d (box)
 {
    dir1 = extract_first_axis_direction (plane);
    dir2 = extract_second_axis_direction (plane);
    /*
    Box3D::Axis axis = (rel_pos == Box3D::FRONT ? Box3D::NONE : Box3D::third_axis_direction (plane));
    set_corners (box->corners[axis],
                 box->corners[axis ^ dir1],
                 box->corners[axis ^ dir1 ^ dir2],
                 box->corners[axis ^ dir2]);
    */
    set_corners (A, B, C, D);
}

Box3DFace::~Box3DFace()
{
    for (int i = 0; i < 4; ++i) {
        if (this->corners[i]) {
            //delete this->corners[i];
            this->corners[i] = NULL;
        }
    }
} 

void Box3DFace::set_corners(NR::Point &A, NR::Point &B, NR::Point &C, NR::Point &D)
{
    corners[0] = &A;
    corners[1] = &B;
    corners[2] = &C;
    corners[3] = &D;
}

/***
void Box3DFace::set_shape(NR::Point const ul, NR::Point const lr,
                     Box3D::Axis const dir1, Box3D::Axis const dir2,
                     unsigned int shift_count, NR::Maybe<NR::Point> pt_align, bool align_along_PL)
{
    corners[0] = ul;
    if (!pt_align) {
        corners[2] = lr;
    } else {
        if (align_along_PL) {
            Box3D::Axis dir3 = Box3D::third_axis_direction (dir1, dir2);
            Box3D::Line line1(*Box3D::Perspective3D::current_perspective->get_vanishing_point(dir1), lr);
            Box3D::Line line2(*pt_align, *Box3D::Perspective3D::current_perspective->get_vanishing_point(dir3));
            corners[2] = *line1.intersect(line2);
        } else {
            corners[2] = Box3D::Line(*pt_align, *Box3D::Perspective3D::current_perspective->get_vanishing_point(dir1)).closest_to(lr);
        }
    }

    Box3D::PerspectiveLine first_line  (corners[0], dir1);
    Box3D::PerspectiveLine second_line (corners[2], dir2);
    NR::Maybe<NR::Point> ur = first_line.intersect(second_line);

    Box3D::PerspectiveLine third_line  (corners[0], dir2);
    Box3D::PerspectiveLine fourth_line (corners[2], dir1);
    NR::Maybe<NR::Point> ll = third_line.intersect(fourth_line);

    // FIXME: How to handle the case if one of the intersections doesn't exist?
    //        Maybe set them equal to the corresponding VPs? 
    if (!ur) ur = NR::Point(0.0, 0.0);    
    if (!ll) ll = NR::Point(0.0, 0.0);

    corners[1] = *ll;
    corners[3] = *ur;

    this->dir1 = dir1;
    this->dir2 = dir2;

    // FIXME: Can be made more concise
    NR::Point tmp_pt;
    for (unsigned int i=0; i < shift_count; i++) {
    	tmp_pt = corners[3];
    	corners[1] = corners[0];
    	corners[2] = corners[1];
    	corners[3] = corners[2];
    	corners[0] = tmp_pt;
    }
}
***/

Box3DFace::Box3DFace(Box3DFace const &box3dface)
{
    for (int i = 0; i < 4; ++i) {
        this->corners[i] = box3dface.corners[i];
    }
    this->dir1 = box3dface.dir1;
    this->dir2 = box3dface.dir2;
}

/**
 * Construct a 3D box face with opposite corners A and C whose sides are directed
 * along axis1 and axis2. The corners have the following order:
 *
 * A = corners[0]  --> along axis1 --> B = corners[1] --> along axis2 --> C = corners[2]
 *                 --> along axis1 --> D = corners[3] --> along axis2 --> D = corners[0].
 * 
 * Note that several other functions rely on this precise order.
 */
/***
void
Box3DFace::set_face (NR::Point const A, NR::Point const C, Box3D::Axis const axis1, Box3D::Axis const axis2)
{
    *corners[0] = A;
    *corners[2] = C;
    if (!SP_IS_3DBOX_CONTEXT(inkscape_active_event_context()))
        return;
    SP3DBoxContext *bc = SP_3DBOX_CONTEXT(inkscape_active_event_context());
    
    Box3D::PerspectiveLine line1 (A, axis1, Box3D::Perspective3D::current_perspective);
    Box3D::PerspectiveLine line2 (C, axis2, Box3D::Perspective3D::current_perspective);
    NR::Maybe<NR::Point> B = line1.intersect(line2);

    Box3D::PerspectiveLine line3 (*corners[0], axis2, Box3D::Perspective3D::current_perspective);
    Box3D::PerspectiveLine line4 (*corners[2], axis1, Box3D::Perspective3D::current_perspective);
    NR::Maybe<NR::Point> D = line3.intersect(line4);

    // FIXME: How to handle the case if one of the intersections doesn't exist?
    //        Maybe set them equal to the corresponding VPs? 
    if (!D) D = NR::Point(0.0, 0.0);    
    if (!B) B = NR::Point(0.0, 0.0);

    *corners[1] = *B;
    *corners[3] = *D;

    this->dir1 = axis1;
    this->dir2 = axis2;
}
***/

NR::Point Box3DFace::operator[](unsigned int i)
{
    return *corners[i % 4];
}



/**
 * Append the curve's path as a child to the given 3D box (since SP3DBox
 * is derived from SPGroup, so we can append children to its svg representation)
 */
void Box3DFace::hook_path_to_3dbox(SPPath * existing_path)
{
    if (this->path) {
        //g_print ("Path already exists. Returning ...\n");
        return;
    }

    if (existing_path != NULL) {
        // no need to create a new path
        this->path = existing_path;
        return;
    }

    /* create new path for face */
    Inkscape::XML::Document *xml_doc = sp_document_repr_doc(SP_OBJECT_DOCUMENT(SP_OBJECT(parent_box3d)));

    Inkscape::XML::Node *repr_face = xml_doc->createElement("svg:path");
    repr_face->setAttribute("inkscape:box3dface", this->axes_string());
    this->path = SP_PATH(SP_OBJECT(parent_box3d)->appendChildRepr(repr_face));
    Inkscape::GC::release(repr_face);

    /* set the correct style */
    this->set_style (repr_face);
}

void Box3DFace::set_style(Inkscape::XML::Node *repr_face, bool extruded)
{
    if (repr_face == NULL) {
        repr_face = SP_OBJECT_REPR (this->path);
    }

    if (!extruded && !strcmp (axes_string (), "XYrear")) {
        // to avoid "flashing" during the initial dragging process, we make the rear face invisible in this case
        repr_face->setAttribute("style", "fill:none");
        return;
    }

    gchar *descr = g_strconcat ("desktop.", axes_string (), NULL);
    const gchar * cur_style = prefs_get_string_attribute(descr, "style");
    g_free (descr);    
    
    SPDesktop *desktop = inkscape_active_desktop();
    bool use_current = prefs_get_int_attribute("tools.shapes.3dbox", "usecurrent", 0);
    if (use_current && cur_style !=NULL) {
        /* use last used style */
        repr_face->setAttribute("style", cur_style);
    } else {
        /* use default style */
        GString *pstring = g_string_new("");
        g_string_printf (pstring, "tools.shapes.3dbox.%s", axes_string());
        sp_desktop_apply_style_tool (desktop, repr_face, pstring->str, false);
    }
}

/**
 * Write the path's "d" attribute to the SVG representation.
 */
void Box3DFace::set_path_repr()
{
    NR::Matrix const i2d (sp_item_i2d_affine (SP_ITEM (this->parent_box3d)));
    SPCurve * curve = sp_curve_new();
    sp_curve_moveto (curve, ((*corners[0]) * i2d)[NR::X], ((*corners[0]) * i2d)[NR::Y]);
    sp_curve_lineto (curve, ((*corners[1]) * i2d)[NR::X], ((*corners[1]) * i2d)[NR::Y]);
    sp_curve_lineto (curve, ((*corners[2]) * i2d)[NR::X], ((*corners[2]) * i2d)[NR::Y]);
    sp_curve_lineto (curve, ((*corners[3]) * i2d)[NR::X], ((*corners[3]) * i2d)[NR::Y]);
    sp_curve_closepath (curve);
    SP_OBJECT(this->path)->repr->setAttribute("d", sp_svg_write_path (SP_CURVE_BPATH(curve)));
}

void Box3DFace::set_curve()
{
    if (this->path == NULL) {
        g_warning("this->path is NULL! \n");
        return;
    }
    NR::Matrix const i2d (sp_item_i2d_affine (SP_ITEM (this->parent_box3d)));
    SPCurve *curve = sp_curve_new();
    sp_curve_moveto(curve, (*corners[0]) * i2d);
    sp_curve_lineto(curve, (*corners[1]) * i2d);
    sp_curve_lineto(curve, (*corners[2]) * i2d);
    sp_curve_lineto(curve, (*corners[3]) * i2d);
    sp_curve_closepath(curve);
    sp_shape_set_curve(SP_SHAPE(this->path), curve, true);
    sp_curve_unref(curve);
}

gchar * Box3DFace::axes_string()
{
    GString *pstring = g_string_new("");
    g_string_printf (pstring, "%s", Box3D::string_from_axes ((Box3D::Axis) (dir1 ^ dir2)));
    switch ((Box3D::Axis) (dir1 ^ dir2)) {
        case Box3D::XY:
            g_string_append_printf (pstring, (front_or_rear == Box3D::FRONT) ? "front" : "rear");
            break;
        case Box3D::XZ:
            g_string_append_printf (pstring, (front_or_rear == Box3D::FRONT) ? "top" : "bottom");
            break;
        case Box3D::YZ:
            g_string_append_printf (pstring, (front_or_rear == Box3D::FRONT) ? "right" : "left");
            break;
        default:
            break;
    }
    return pstring->str;
}

gint Box3DFace::descr_to_id (gchar const *descr)
{
    if (!strcmp (descr, "XYrear")) { return 5; }
    if (!strcmp (descr, "XYfront")) { return 4; }
    if (!strcmp (descr, "XZbottom")) { return 3; }
    if (!strcmp (descr, "XZtop")) { return 2; }
    if (!strcmp (descr, "YZleft")) { return 1; }
    if (!strcmp (descr, "YZright")) { return 0; }

    g_warning ("Invalid description of 3D box face.\n");
    return -1;
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
