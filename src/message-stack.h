/*
 * Inkscape::MessageStack - class for mangaging current status messages
 *
 * Authors:
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_MESSAGE_STACK_H
#define SEEN_INKSCAPE_MESSAGE_STACK_H

#include <sigc++/sigc++.h>
#include <glib/gtypes.h>
#include <stdarg.h>
#include "refcounted.h"
#include "message.h"

namespace Inkscape {

/**
 * A class which holds a stack of displayed messages.  Messages
 * can be pushed onto the top of the stack, and removed from any
 * point in the stack by their id.
 *
 * Messages may also be "flashed", meaning that they will be
 * automatically removed from the stack a fixed period of time
 * after they are pushed.
 *
 * "Flashed" warnings and errors will persist longer than normal
 * messages.
 *
 * There is no simple "pop" operation provided, since these
 * stacks are intended to be shared by many different clients;
 * assuming that the message you pushed is still on top is an
 * invalid and unsafe assumption.
 */
class MessageStack : public Refcounted {
public:
    MessageStack();
    ~MessageStack();

    /** @brief returns the type of message currently at the top of the stack */
    MessageType currentMessageType() {
        return _messages ? _messages->type : NORMAL_MESSAGE;
    }
    /** @brief returns the text of the message currently at the top of
      *        the stack
      */
    gchar const *currentMessage() {
        return _messages ? _messages->message : NULL;
    }

    /** @brief connects to the "changed" signal which is emitted whenever
      *        the topmost message on the stack changes.
      */
    SigC::Connection connectChanged(SigC::Slot2<void, MessageType, gchar const *> slot)
    {
        return _changed_signal.connect(slot);
    }

    /** @brief pushes a message onto the stack
      *
      * @param type the message type
      * @param message the message text
      *
      * @return the id of the pushed message
      */
    MessageId push(MessageType type, gchar const *message);

    /** @brief pushes a message onto the stack using printf-like formatting
      *
      * @param type the message type
      * @param format a printf-style format string
      *
      * @return the id of the pushed message
      */
    MessageId pushF(MessageType type, gchar const *format, ...);

    /** @brief pushes a message onto the stack using printf-like formatting,
      *        using a stdarg argument list
      *
      * @param type the message type
      * @param format a printf-style format string
      * @param args the subsequent printf-style arguments
      *
      * @return the id of the pushed message
      */
    MessageId pushVF(MessageType type, gchar const *format, va_list args);

    /** @brief removes a message from the stack, given its id
      *
      * This method will remove a message from the stack if it has not
      * already been removed.  It may be removed from any part of the stack.
      * 
      * @param id the message id to remove
      */
    void cancel(MessageId id);

    /** @brief temporarily pushes a message onto the stack
      *
      * @param type the message type
      * @param message the message text
      */
    void flash(MessageType type, gchar const *message);

    /** @brief temporarily pushes a message onto the stack using
      *        printf-like formatting
      *
      * @param type the message type
      * @param format a printf-style format string
      */
    void flashF(MessageType type, gchar const *format, ...);

    /** @brief temporarily pushes a message onto the stack using
      *        printf-like formatting, using a stdarg argument list
      *
      * @param type the message type
      * @param format a printf-style format string
      * @param args the printf-style arguments
      */
    void flashVF(MessageType type, gchar const *format, va_list args);

private:
    struct Message {
        Message *next;
        MessageStack *stack;
        MessageId id;
        MessageType type;
        gchar *message;
        guint timeout_id;
    };

    MessageStack(MessageStack const &); // no copy
    void operator=(MessageStack const &); // no assign

    /// pushes a message onto the stack with an optional timeout
    MessageId _push(MessageType type, guint lifetime, gchar const *message);

    Message *_discard(Message *m); ///< frees a message struct and returns the next such struct in the list
    void _emitChanged(); ///< emits the "changed" signal
    static gboolean _timeout(gpointer data); ///< callback to expire flashed messages

    SigC::Signal2<void, MessageType, gchar const *> _changed_signal;
    Message *_messages; ///< the stack of messages as a linked list
    MessageId _next_id; ///< the next message id to assign
};

}

#endif
/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
