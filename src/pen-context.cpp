/** \file 
 * Pen event context implementation. 
 */

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2000 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 * Copyright (C) 2002 Lauris Kaplinski
 * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gdk/gdkkeysyms.h>

#include "pen-context.h"
#include "sp-namedview.h"
#include "sp-metrics.h"
#include "desktop.h"
#include "desktop-affine.h"
#include "desktop-handles.h"
#include "selection.h"
#include "draw-anchor.h"
#include "message-stack.h"
#include "message-context.h"
#include "event-context.h"
#include "prefs-utils.h"
#include "sp-item.h"
#include "sp-path.h"

#include "pixmaps/cursor-pen.xpm"
#include "display/canvas-bpath.h"
#include "display/sp-canvas.h"
#include "display/sp-ctrlline.h"
#include "display/sodipodi-ctrl.h"
#include <glibmm/i18n.h>
#include "libnr/nr-point-ops.h"
#include "libnr/n-art-bpath.h"
#include "helper/units.h"
#include "snap.h"
#include "macros.h"


static void sp_pen_context_class_init(SPPenContextClass *klass);
static void sp_pen_context_init(SPPenContext *pc);
static void sp_pen_context_dispose(GObject *object);

static void sp_pen_context_setup(SPEventContext *ec);
static void sp_pen_context_finish(SPEventContext *ec);
static void sp_pen_context_set(SPEventContext *ec, gchar const *key, gchar const *val);
static gint sp_pen_context_root_handler(SPEventContext *ec, GdkEvent *event);

static void spdc_pen_set_initial_point(SPPenContext *pc, NR::Point const p);
static void spdc_pen_set_subsequent_point(SPPenContext *pc, NR::Point const p, bool statusbar);
static void spdc_pen_set_ctrl(SPPenContext *pc, NR::Point const p, guint state);
static void spdc_pen_finish_segment(SPPenContext *pc, NR::Point p, guint state);

static void spdc_pen_finish(SPPenContext *pc, gboolean closed);

static gint pen_handle_button_press(SPPenContext *const pc, GdkEventButton const &bevent);
static gint pen_handle_motion_notify(SPPenContext *const pc, GdkEventMotion const &mevent);
static gint pen_handle_button_release(SPPenContext *const pc, GdkEventButton const &revent);
static gint pen_handle_2button_press(SPPenContext *const pc);
static gint pen_handle_key_press(SPPenContext *const pc, GdkEvent *event);
static void spdc_reset_colors(SPPenContext *pc);


static NR::Point pen_drag_origin_w(0, 0);
static bool pen_within_tolerance = false;

static SPDrawContextClass *pen_parent_class;

/**
 * Register SPPenContext with Gdk and return its type.
 */
GType
sp_pen_context_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPPenContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_pen_context_class_init,
            NULL, NULL,
            sizeof(SPPenContext),
            4,
            (GInstanceInitFunc) sp_pen_context_init,
            NULL,   /* value_table */
        };
        type = g_type_register_static(SP_TYPE_DRAW_CONTEXT, "SPPenContext", &info, (GTypeFlags)0);
    }
    return type;
}

/**
 * Initialize the SPPenContext vtable.
 */
static void
sp_pen_context_class_init(SPPenContextClass *klass)
{
    GObjectClass *object_class;
    SPEventContextClass *event_context_class;

    object_class = (GObjectClass *) klass;
    event_context_class = (SPEventContextClass *) klass;

    pen_parent_class = (SPDrawContextClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_pen_context_dispose;

    event_context_class->setup = sp_pen_context_setup;
    event_context_class->finish = sp_pen_context_finish;
    event_context_class->set = sp_pen_context_set;
    event_context_class->root_handler = sp_pen_context_root_handler;
}

/**
 * Callback to initialize SPPenContext object.
 */
static void
sp_pen_context_init(SPPenContext *pc)
{

    SPEventContext *event_context = SP_EVENT_CONTEXT(pc);

    event_context->cursor_shape = cursor_pen_xpm;
    event_context->hot_x = 4;
    event_context->hot_y = 4;
    
    pc->npoints = 0;
    pc->mode = SP_PEN_CONTEXT_MODE_CLICK;
    pc->state = SP_PEN_CONTEXT_POINT;

    pc->c0 = NULL;
    pc->c1 = NULL;
    pc->cl0 = NULL;
    pc->cl1 = NULL;
}

/**
 * Callback to destroy the SPPenContext object's members and itself.
 */
static void
sp_pen_context_dispose(GObject *object)
{
    SPPenContext *pc;

    pc = SP_PEN_CONTEXT(object);

    if (pc->c0) {
        gtk_object_destroy(GTK_OBJECT(pc->c0));
        pc->c0 = NULL;
    }
    if (pc->c1) {
        gtk_object_destroy(GTK_OBJECT(pc->c1));
        pc->c1 = NULL;
    }
    if (pc->cl0) {
        gtk_object_destroy(GTK_OBJECT(pc->cl0));
        pc->cl0 = NULL;
    }
    if (pc->cl1) {
        gtk_object_destroy(GTK_OBJECT(pc->cl1));
        pc->cl1 = NULL;
    }

    G_OBJECT_CLASS(pen_parent_class)->dispose(object);
}

/**
 * Callback to initialize SPPenContext object.
 */
static void
sp_pen_context_setup(SPEventContext *ec)
{
    SPPenContext *pc;

    pc = SP_PEN_CONTEXT(ec);

    if (((SPEventContextClass *) pen_parent_class)->setup) {
        ((SPEventContextClass *) pen_parent_class)->setup(ec);
    }

    /* Pen indicators */
    pc->c0 = sp_canvas_item_new(SP_DT_CONTROLS(SP_EVENT_CONTEXT_DESKTOP(ec)), SP_TYPE_CTRL, "shape", SP_CTRL_SHAPE_CIRCLE,
                                "size", 4.0, "filled", 0, "fill_color", 0xff00007f, "stroked", 1, "stroke_color", 0x0000ff7f, NULL);
    pc->c1 = sp_canvas_item_new(SP_DT_CONTROLS(SP_EVENT_CONTEXT_DESKTOP(ec)), SP_TYPE_CTRL, "shape", SP_CTRL_SHAPE_CIRCLE,
                                "size", 4.0, "filled", 0, "fill_color", 0xff00007f, "stroked", 1, "stroke_color", 0x0000ff7f, NULL);
    pc->cl0 = sp_canvas_item_new(SP_DT_CONTROLS(SP_EVENT_CONTEXT_DESKTOP(ec)), SP_TYPE_CTRLLINE, NULL);
    sp_ctrlline_set_rgba32(SP_CTRLLINE(pc->cl0), 0x0000007f);
    pc->cl1 = sp_canvas_item_new(SP_DT_CONTROLS(SP_EVENT_CONTEXT_DESKTOP(ec)), SP_TYPE_CTRLLINE, NULL);
    sp_ctrlline_set_rgba32(SP_CTRLLINE(pc->cl1), 0x0000007f);

    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);

    sp_event_context_read(ec, "mode");

    pc->anchor_statusbar = false;

    if (prefs_get_int_attribute("tools.freehand.pen", "selcue", 0) != 0) {
        ec->enableSelectionCue();
    }
}

/**
 * Finalization callback.
 */
static void
sp_pen_context_finish(SPEventContext *ec)
{
    spdc_pen_finish(SP_PEN_CONTEXT(ec), FALSE);

    if (((SPEventContextClass *) pen_parent_class)->finish) {
        ((SPEventContextClass *) pen_parent_class)->finish(ec);
    }
}

/**
 * Callback that sets key to value in pen context.
 */
static void
sp_pen_context_set(SPEventContext *ec, gchar const *key, gchar const *val)
{
    SPPenContext *pc = SP_PEN_CONTEXT(ec);

    if (!strcmp(key, "mode")) {
        if ( val && !strcmp(val, "drag") ) {
            pc->mode = SP_PEN_CONTEXT_MODE_DRAG;
        } else {
            pc->mode = SP_PEN_CONTEXT_MODE_CLICK;
        }
    }
}

/** 
 * Snaps new node relative to the previous node.
 */
static void
spdc_endpoint_snap(SPPenContext const *const pc, NR::Point &p, guint const state)
{
    if (pc->npoints > 0) {
        spdc_endpoint_snap_rotation(pc, p, pc->p[0], state);
    }

    spdc_endpoint_snap_free(pc, p, state);
}

/** 
 * Snaps new node's handle relative to the new node.
 */
static void
spdc_endpoint_snap_handle(SPPenContext const *const pc, NR::Point &p, guint const state)
{
    g_return_if_fail(( pc->npoints == 2 ||
                       pc->npoints == 5   ));

    spdc_endpoint_snap_rotation(pc, p, pc->p[pc->npoints - 2], state);
    spdc_endpoint_snap_free(pc, p, state);
}

/**
 * Callback to handle all pen events.
 */
static gint
sp_pen_context_root_handler(SPEventContext *ec, GdkEvent *event)
{
    SPPenContext *const pc = SP_PEN_CONTEXT(ec);

    gint ret = FALSE;

    switch (event->type) {
        case GDK_BUTTON_PRESS:
            ret = pen_handle_button_press(pc, event->button);
            break;

        case GDK_MOTION_NOTIFY:
            ret = pen_handle_motion_notify(pc, event->motion);
            break;

        case GDK_BUTTON_RELEASE:
            ret = pen_handle_button_release(pc, event->button);
            break;

        case GDK_2BUTTON_PRESS:
            ret = pen_handle_2button_press(pc);
            break;

        case GDK_KEY_PRESS:
            ret = pen_handle_key_press(pc, event);
            break;

        default:
            break;
    }

    if (!ret) {
        gint (*const parent_root_handler)(SPEventContext *, GdkEvent *)
            = ((SPEventContextClass *) pen_parent_class)->root_handler;
        if (parent_root_handler) {
            ret = parent_root_handler(ec, event);
        }
    }

    return ret;
}

/**
 * Handle mouse button press event.
 */
static gint pen_handle_button_press(SPPenContext *const pc, GdkEventButton const &bevent)
{
    gint ret = FALSE;
    if (bevent.button == 1) {

        SPDrawContext * const dc = SP_DRAW_CONTEXT(pc);
        SPDesktop * const desktop = SP_EVENT_CONTEXT_DESKTOP(dc);
        SPItem * const layer = SP_ITEM(desktop->currentLayer());
        
        if (!layer || desktop->itemIsHidden(layer)) {
            dc->_message_context->flash(
                Inkscape::WARNING_MESSAGE, _("<b>Current layer is hidden</b>. Unhide it to be able to draw on it.")
                );
            return TRUE;
        }
        
        if (!layer || layer->isLocked()) {
            dc->_message_context->flash(
                Inkscape::WARNING_MESSAGE, _("<b>Current layer is locked</b>. Unlock it to be able to draw on it.")
                );
            return TRUE;
        }

        NR::Point const event_w(bevent.x, bevent.y);
        pen_drag_origin_w = event_w;
        pen_within_tolerance = true;

        /* Test whether we hit any anchor. */
        SPDrawAnchor * const anchor = spdc_test_inside(pc, event_w);

        NR::Point const event_dt(sp_desktop_w2d_xy_point(desktop, event_w));
        switch (pc->mode) {
            case SP_PEN_CONTEXT_MODE_CLICK:
                /* In click mode we add point on release */
                switch (pc->state) {
                    case SP_PEN_CONTEXT_POINT:
                    case SP_PEN_CONTEXT_CONTROL:
                    case SP_PEN_CONTEXT_CLOSE:
                        break;
                    case SP_PEN_CONTEXT_STOP:
                        /* This is allowed, if we just cancelled curve */
                        pc->state = SP_PEN_CONTEXT_POINT;
                        break;
                    default:
                        break;
                }
                break;
            case SP_PEN_CONTEXT_MODE_DRAG:
                switch (pc->state) {
                    case SP_PEN_CONTEXT_STOP:
                        /* This is allowed, if we just cancelled curve */
                    case SP_PEN_CONTEXT_POINT:
                        if (pc->npoints == 0) {
                            
                            /* Set start anchor */
                            pc->sa = anchor;
                            NR::Point p;
                            if (anchor) {
                                
                                /* Adjust point to anchor if needed */
                                p = anchor->dp;
                                desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Continuing selected path"));

                            } else {

                                // This is the first click of a new curve; deselect item so that
                                // this curve is not combined with it (unless it is drawn from its
                                // anchor, which is handled by the sibling branch above)
                                Inkscape::Selection * const selection = SP_DT_SELECTION(desktop);
                                if (!(bevent.state & GDK_SHIFT_MASK)) {
                                    
                                    selection->clear();
                                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Creating new path"));

                                } else if (selection->singleItem() && SP_IS_PATH(selection->singleItem())) {

                                    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Appending to selected path"));
                                }

                                /* Create green anchor */
                                p = event_dt;
                                spdc_endpoint_snap(pc, p, bevent.state);
                                pc->green_anchor = sp_draw_anchor_new(pc, pc->green_curve, TRUE, p);
                            }
                            spdc_pen_set_initial_point(pc, p);
                        } else {
                            
                            /* Set end anchor */
                            pc->ea = anchor;
                            NR::Point p;
                            if (anchor) {
                                
                                p = anchor->dp;
                                // we hit an anchor, will finish the curve (either with or without closing)
                                // in release handler
                                pc->state = SP_PEN_CONTEXT_CLOSE;

                                if (pc->green_anchor && pc->green_anchor->active) {
                                    // we clicked on the current curve start, so close it even if
                                    // we drag a handle away from it
                                    dc->green_closed = TRUE; 
                                }
                                ret = TRUE;
                                break;
                                
                            } else {
                                
                                p = event_dt;
                                spdc_endpoint_snap(pc, p, bevent.state); /* Snap node only if not hitting anchor. */
                                spdc_pen_set_subsequent_point(pc, p, true);
                            }
                            
                        }
                        pc->state = SP_PEN_CONTEXT_CONTROL;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_CONTROL:
                        g_warning("Button down in CONTROL state");
                        break;
                    case SP_PEN_CONTEXT_CLOSE:
                        g_warning("Button down in CLOSE state");
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    } else if (bevent.button == 3) {
        if (pc->npoints != 0) {
            spdc_pen_finish(pc, FALSE);
            ret = TRUE;
        }
    }
    
    return ret;
}

/**
 * Handle motion_notify event.
 */
static gint
pen_handle_motion_notify(SPPenContext *const pc, GdkEventMotion const &mevent)
{
    gint ret = FALSE;

    if (mevent.state & GDK_BUTTON2_MASK || mevent.state & GDK_BUTTON3_MASK) {
        // allow middle-button scrolling
        return FALSE;
    }

    NR::Point const event_w(mevent.x,
                            mevent.y);
    if (pen_within_tolerance) {
        gint const tolerance = prefs_get_int_attribute_limited("options.dragtolerance",
                                                               "value", 0, 0, 100);
        if ( NR::LInfty( event_w - pen_drag_origin_w ) < tolerance ) {
            return FALSE;   // Do not drag if we're within tolerance from origin.
        }
    }
    // Once the user has moved farther than tolerance from the original location
    // (indicating they intend to move the object, not click), then always process the
    // motion notify coordinates as given (no snapping back to origin)
    pen_within_tolerance = false;

    SPDesktop *const dt = pc->desktop;
    if ( ( mevent.state & GDK_BUTTON1_MASK ) && !pc->grab ) {
        /* Grab mouse, so release will not pass unnoticed */
        pc->grab = SP_CANVAS_ITEM(dt->acetate);
        sp_canvas_item_grab(pc->grab, ( GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK   |
                                        GDK_BUTTON_RELEASE_MASK |
                                        GDK_POINTER_MOTION_MASK  ),
                            NULL, mevent.time);
    }

    /* Find desktop coordinates */
    NR::Point p = sp_desktop_w2d_xy_point(dt, event_w);

    /* Test, whether we hit any anchor */
    SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);

    switch (pc->mode) {
        case SP_PEN_CONTEXT_MODE_CLICK:
            switch (pc->state) {
                case SP_PEN_CONTEXT_POINT:
                    if ( pc->npoints != 0 ) {
                        /* Only set point, if we are already appending */
                        spdc_endpoint_snap(pc, p, mevent.state);
                        spdc_pen_set_subsequent_point(pc, p, true);
                        ret = TRUE;
                    }
                    break;
                case SP_PEN_CONTEXT_CONTROL:
                case SP_PEN_CONTEXT_CLOSE:
                    /* Placing controls is last operation in CLOSE state */
                    spdc_endpoint_snap(pc, p, mevent.state);
                    spdc_pen_set_ctrl(pc, p, mevent.state);
                    ret = TRUE;
                    break;
                case SP_PEN_CONTEXT_STOP:
                    /* This is perfectly valid */
                    break;
                default:
                    break;
            }
            break;
        case SP_PEN_CONTEXT_MODE_DRAG:
            switch (pc->state) {
                case SP_PEN_CONTEXT_POINT:
                    if ( pc->npoints > 0 ) {
                        /* Only set point, if we are already appending */

                        if (!anchor) {   /* Snap node only if not hitting anchor */
                            spdc_endpoint_snap(pc, p, mevent.state);
                        }

                        spdc_pen_set_subsequent_point(pc, p, !anchor);

                        if (anchor && !pc->anchor_statusbar) {
                            pc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to close and finish the path."));
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->_message_context->clear();
                            pc->anchor_statusbar = false;
                        }

                        ret = TRUE;
                    } else {
                        if (anchor && !pc->anchor_statusbar) {
                            pc->_message_context->set(Inkscape::NORMAL_MESSAGE, _("<b>Click</b> or <b>click and drag</b> to continue the path from this point."));
                            pc->anchor_statusbar = true;
                        } else if (!anchor && pc->anchor_statusbar) {
                            pc->_message_context->clear();
                            pc->anchor_statusbar = false;
                        }
                    }
                    break;
                case SP_PEN_CONTEXT_CONTROL:
                case SP_PEN_CONTEXT_CLOSE:
                    /* Placing controls is last operation in CLOSE state */

                    // snap the handle
                    spdc_endpoint_snap_handle(pc, p, mevent.state);

                    spdc_pen_set_ctrl(pc, p, mevent.state);
                    ret = TRUE;
                    break;
                case SP_PEN_CONTEXT_STOP:
                    /* This is perfectly valid */
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return ret;
}

/**
 * Handle mouse button release event.
 */
static gint
pen_handle_button_release(SPPenContext *const pc, GdkEventButton const &revent)
{
    gint ret = FALSE;
    if ( revent.button == 1 ) {

        SPDrawContext *dc = SP_DRAW_CONTEXT (pc);

        NR::Point const event_w(revent.x,
                                revent.y);
        /* Find desktop coordinates */
        NR::Point p = sp_desktop_w2d_xy_point(pc->desktop, event_w);

        /* Test whether we hit any anchor. */
        SPDrawAnchor *anchor = spdc_test_inside(pc, event_w);

        switch (pc->mode) {
            case SP_PEN_CONTEXT_MODE_CLICK:
                switch (pc->state) {
                    case SP_PEN_CONTEXT_POINT:
                        if ( pc->npoints == 0 ) {
                            /* Start new thread only with button release */
                            if (anchor) {
                                p = anchor->dp;
                            }
                            pc->sa = anchor;
                            spdc_pen_set_initial_point(pc, p);
                        } else {
                            /* Set end anchor here */
                            pc->ea = anchor;
                            if (anchor) {
                                p = anchor->dp;
                            }
                        }
                        pc->state = SP_PEN_CONTEXT_CONTROL;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_CONTROL:
                        /* End current segment */
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        pc->state = SP_PEN_CONTEXT_POINT;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_CLOSE:
                        /* End current segment */
                        if (!anchor) {   /* Snap node only if not hitting anchor */
                            spdc_endpoint_snap(pc, p, revent.state);
                        }
                        spdc_pen_finish_segment(pc, p, revent.state);
                        spdc_pen_finish(pc, TRUE);
                        pc->state = SP_PEN_CONTEXT_POINT;
                        ret = TRUE;
                        break;
                    case SP_PEN_CONTEXT_STOP:
                        /* This is allowed, if we just cancelled curve */
                        pc->state = SP_PEN_CONTEXT_POINT;
                        ret = TRUE;
                        break;
                    default:
                        break;
                }
                break;
            case SP_PEN_CONTEXT_MODE_DRAG:
                switch (pc->state) {
                    case SP_PEN_CONTEXT_POINT:
                    case SP_PEN_CONTEXT_CONTROL:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        break;
                    case SP_PEN_CONTEXT_CLOSE:
                        spdc_endpoint_snap(pc, p, revent.state);
                        spdc_pen_finish_segment(pc, p, revent.state);
                        if (pc->green_closed) {
                            // finishing at the start anchor, close curve
                            spdc_pen_finish(pc, TRUE);
                        } else {
                            // finishing at some other anchor, finish curve but not close
                            spdc_pen_finish(pc, FALSE);
                        }
                        break;
                    case SP_PEN_CONTEXT_STOP:
                        /* This is allowed, if we just cancelled curve */
                        break;
                    default:
                        break;
                }
                pc->state = SP_PEN_CONTEXT_POINT;
                ret = TRUE;
                break;
            default:
                break;
        }

        if (pc->grab) {
            /* Release grab now */
            sp_canvas_item_ungrab(pc->grab, revent.time);
            pc->grab = NULL;
        }

        ret = TRUE;

        dc->green_closed = FALSE;
    }

    return ret;
}

static gint
pen_handle_2button_press(SPPenContext *const pc)
{
    gint ret = FALSE;
    if (pc->npoints != 0) {
        spdc_pen_finish(pc, FALSE);
        ret = TRUE;
    }
    return ret;
}

static gint
pen_handle_key_press(SPPenContext *const pc, GdkEvent *event)
{
    gint ret = FALSE;
    /* fixme: */
    switch (get_group0_keyval (&event->key)) {
        case GDK_Return:
        case GDK_KP_Enter:
            if (pc->npoints != 0) {
                spdc_pen_finish(pc, FALSE);
                ret = TRUE;
            }
            break;
        case GDK_Escape:
            if (pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for deselecting
                pc->state = SP_PEN_CONTEXT_STOP;
                spdc_reset_colors(pc);
                sp_canvas_item_hide(pc->c0);
                sp_canvas_item_hide(pc->c1);
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                ret = TRUE;
            }
            break;
        case GDK_z:
        case GDK_Z:
            if (MOD__CTRL_ONLY && pc->npoints != 0) {
                // if drawing, cancel, otherwise pass it up for undo
                pc->state = SP_PEN_CONTEXT_STOP;
                spdc_reset_colors(pc);
                sp_canvas_item_hide(pc->c0);
                sp_canvas_item_hide(pc->c1);
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                ret = TRUE;
            }
            break;
        case GDK_BackSpace:
        case GDK_Delete:
        case GDK_KP_Delete:
            if (sp_curve_is_empty(pc->green_curve)) {
                /* Same as cancel */
                pc->state = SP_PEN_CONTEXT_STOP;
                spdc_reset_colors(pc);
                sp_canvas_item_hide(pc->c0);
                sp_canvas_item_hide(pc->c1);
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                ret = TRUE;
            } else {
                /* Reset red curve */
                sp_curve_reset(pc->red_curve);
                /* Destroy topmost green bpath */
                if (pc->green_bpaths) {
                    if (pc->green_bpaths->data)
                        gtk_object_destroy(GTK_OBJECT(pc->green_bpaths->data));
                    pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
                }    
                /* Get last segment */
                NArtBpath const *const p = SP_CURVE_BPATH(pc->green_curve);
                gint const e = SP_CURVE_LENGTH(pc->green_curve);
                if ( e < 2 ) {
                    g_warning("Green curve length is %d", e);
                    break;
                }
                pc->p[0] = p[e - 2].c(3);
                pc->p[1] = p[e - 1].c(1);
                NR::Point const pt(( pc->npoints < 4
                                     ? p[e - 1].c(3)
                                     : pc->p[3] ));
                pc->npoints = 2;
                sp_curve_backspace(pc->green_curve);
                sp_canvas_item_hide(pc->c0);
                sp_canvas_item_hide(pc->c1);
                sp_canvas_item_hide(pc->cl0);
                sp_canvas_item_hide(pc->cl1);
                pc->state = SP_PEN_CONTEXT_POINT;
                spdc_pen_set_subsequent_point(pc, pt, true);
                ret = TRUE;
            }
            break;
        default:
            break;
    }
    return ret;
}

static void
spdc_reset_colors(SPPenContext *pc)
{
    /* Red */
    sp_curve_reset(pc->red_curve);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), NULL);
    /* Blue */
    sp_curve_reset(pc->blue_curve);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->blue_bpath), NULL);
    /* Green */
    while (pc->green_bpaths) {
        gtk_object_destroy(GTK_OBJECT(pc->green_bpaths->data));
        pc->green_bpaths = g_slist_remove(pc->green_bpaths, pc->green_bpaths->data);
    }
    sp_curve_reset(pc->green_curve);
    if (pc->green_anchor) {
        pc->green_anchor = sp_draw_anchor_destroy(pc->green_anchor);
    }
    pc->sa = NULL;
    pc->ea = NULL;
    pc->npoints = 0;
    pc->red_curve_is_valid = false;
}


static void
spdc_pen_set_initial_point(SPPenContext *const pc, NR::Point const p)
{
    g_assert( pc->npoints == 0 );

    pc->p[0] = p;
    pc->p[1] = p;
    pc->npoints = 2;
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), NULL);
}

static void
spdc_pen_set_subsequent_point(SPPenContext *const pc, NR::Point const p, bool statusbar)
{
    g_assert( pc->npoints != 0 );
    /* todo: Check callers to see whether 2 <= npoints is guaranteed. */

    pc->p[2] = p;
    pc->p[3] = p;
    pc->p[4] = p;
    pc->npoints = 5;
    sp_curve_reset(pc->red_curve);
    sp_curve_moveto(pc->red_curve, pc->p[0]);
    bool is_curve;
    if ( (pc->onlycurves)
         || ( pc->p[1] != pc->p[0] ) )
    {
        sp_curve_curveto(pc->red_curve, pc->p[1], p, p);
        is_curve = true;
    } else {
        sp_curve_lineto(pc->red_curve, p);
        is_curve = false;
    }
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);

    if (statusbar) {
        // status text
        SPDesktop *desktop = SP_EVENT_CONTEXT(pc)->desktop;
        NR::Point rel = p - pc->p[0];
        GString *dist = SP_PX_TO_METRIC_STRING(NR::L2(rel), desktop->namedview->getDefaultMetric());
        double angle = atan2(rel[NR::Y], rel[NR::X]) * 180 / M_PI;
        if (prefs_get_int_attribute("options.compassangledisplay", "value", 0) != 0)
            angle = angle_to_compass (angle);
        pc->_message_context->setF(Inkscape::NORMAL_MESSAGE, _("<b>%s</b>: angle %3.2f&#176;, distance %s; with <b>Ctrl</b> to snap angle, <b>Enter</b> to finish the path"), is_curve? "Curve segment" : "Line segment", angle, dist->str);
        g_string_free(dist, FALSE);
    }
}

static void
spdc_pen_set_ctrl(SPPenContext *const pc, NR::Point const p, guint const state)
{
    sp_canvas_item_show(pc->c1);
    sp_canvas_item_show(pc->cl1);

    if ( pc->npoints == 2 ) {
        pc->p[1] = p;
        sp_canvas_item_hide(pc->c0);
        sp_canvas_item_hide(pc->cl0);
        SP_CTRL(pc->c1)->moveto(pc->p[1]);
        sp_ctrlline_set_coords(SP_CTRLLINE(pc->cl1), pc->p[0], pc->p[1]);

        // status text
        SPDesktop *desktop = SP_EVENT_CONTEXT(pc)->desktop;
        NR::Point rel = p - pc->p[0];
        GString *dist = SP_PX_TO_METRIC_STRING(NR::L2(rel), desktop->namedview->getDefaultMetric());
        double angle = atan2(rel[NR::Y], rel[NR::X]) * 180 / M_PI;
        if (prefs_get_int_attribute("options.compassangledisplay", "value", 0) != 0)
            angle = angle_to_compass (angle);
        pc->_message_context->setF(Inkscape::NORMAL_MESSAGE, _("<b>Curve handle</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle"), angle, dist->str);
        g_string_free(dist, FALSE);

    } else if ( pc->npoints == 5 ) {
        pc->p[4] = p;
        sp_canvas_item_show(pc->c0);
        sp_canvas_item_show(pc->cl0);
        bool is_symm = false;
        if ( ( ( pc->mode == SP_PEN_CONTEXT_MODE_CLICK ) && ( state & GDK_CONTROL_MASK ) ) ||
             ( ( pc->mode == SP_PEN_CONTEXT_MODE_DRAG ) &&  !( state & GDK_SHIFT_MASK  ) ) ) {
            NR::Point delta = p - pc->p[3];
            pc->p[2] = pc->p[3] - delta;
            is_symm = true;
            sp_curve_reset(pc->red_curve);
            sp_curve_moveto(pc->red_curve, pc->p[0]);
            sp_curve_curveto(pc->red_curve, pc->p[1], pc->p[2], pc->p[3]);
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(pc->red_bpath), pc->red_curve);
        }
        SP_CTRL(pc->c0)->moveto(pc->p[2]);
        sp_ctrlline_set_coords(SP_CTRLLINE(pc->cl0), pc->p[3], pc->p[2]);
        SP_CTRL(pc->c1)->moveto(pc->p[4]);
        sp_ctrlline_set_coords(SP_CTRLLINE(pc->cl1), pc->p[3], pc->p[4]);

        // status text
        SPDesktop *desktop = SP_EVENT_CONTEXT(pc)->desktop;
        NR::Point rel = p - pc->p[3];
        GString *dist = SP_PX_TO_METRIC_STRING(NR::L2(rel), desktop->namedview->getDefaultMetric());
        double angle = atan2(rel[NR::Y], rel[NR::X]) * 180 / M_PI;
        if (prefs_get_int_attribute("options.compassangledisplay", "value", 0) != 0)
            angle = angle_to_compass (angle);
        pc->_message_context->setF(Inkscape::NORMAL_MESSAGE, _("<b>%s</b>: angle %3.2f&#176;, length %s; with <b>Ctrl</b> to snap angle, with <b>Shift</b> to move this handle only"), is_symm? "Curve handle, symmetric" : "Curve handle", angle, dist->str);
        g_string_free(dist, FALSE);

    } else {
        g_warning("Something bad happened - npoints is %d", pc->npoints);
    }
}

static void
spdc_pen_finish_segment(SPPenContext *const pc, NR::Point const p, guint const state)
{
    if (!sp_curve_empty(pc->red_curve)) {
        sp_curve_append_continuous(pc->green_curve, pc->red_curve, 0.0625);
        SPCurve *curve = sp_curve_copy(pc->red_curve);
        /// \todo fixme: 
        SPCanvasItem *cshape = sp_canvas_bpath_new(SP_DT_SKETCH(pc->desktop), curve);
        sp_curve_unref(curve);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cshape), pc->green_color, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);

        pc->green_bpaths = g_slist_prepend(pc->green_bpaths, cshape);

        pc->p[0] = pc->p[3];
        pc->p[1] = pc->p[4];
        pc->npoints = 2;

        sp_curve_reset(pc->red_curve);
    }
}

static void
spdc_pen_finish(SPPenContext *const pc, gboolean const closed)
{
    SPDesktop *const desktop = pc->desktop;
    pc->_message_context->clear();
    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Finishing pen"));

    sp_curve_reset(pc->red_curve);
    spdc_concat_colors_and_flush(pc, closed);
    pc->sa = NULL;
    pc->ea = NULL;

    pc->npoints = 0;
    pc->state = SP_PEN_CONTEXT_POINT;

    sp_canvas_item_hide(pc->c0);
    sp_canvas_item_hide(pc->c1);
    sp_canvas_item_hide(pc->cl0);
    sp_canvas_item_hide(pc->cl1);

    if (pc->green_anchor) {
        pc->green_anchor = sp_draw_anchor_destroy(pc->green_anchor);
    }
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
