/**
 * Whiteboard session manager
 * Jabber received message processors
 *
 * Authors:
 * David Yip <yipdw@rose-hulman.edu>
 *
 * Copyright (c) 2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef __WHITEBOARD_MESSAGE_PROCESSORS_H__
#define __WHITEBOARD_MESSAGE_PROCESSORS_H__

#include "jabber_whiteboard/typedefs.h"

#include "gc-managed.h"
#include "gc-finalized.h"

namespace Inkscape {

namespace Whiteboard {

class SessionManager;

// Processor forward declarations
struct ChangeHandler;
struct DocumentSignalHandler;
struct ConnectRequestHandler;
struct ConnectErrorHandler;
struct ChatSynchronizeHandler;

struct JabberMessage {
public:
	JabberMessage(LmMessage* m) : message(m), sequence(0)
	{	
		lm_message_ref(this->message);
	}

	~JabberMessage() 
	{
		lm_message_unref(this->message);
	}

	// pointer to original Loudmouth message
	LmMessage* message;
	
	// sequence number
	unsigned int sequence;
	
	// sender JID
	std::string sender;

	// message body
	Glib::ustring body;

private:
	// noncopyable, nonassignable (for now, anyway...)
//	JabberMessage(JabberMessage const&);
//	JabberMessage& operator=(JabberMessage const&);
};

struct MessageProcessor : public GC::Managed<>, public GC::Finalized {
public:
	virtual ~MessageProcessor() 
	{

	}

	virtual LmHandlerResult operator()(MessageType mode, JabberMessage& m) = 0;

	MessageProcessor(SessionManager* sm) : _sm(sm) { }
protected:
	SessionManager *_sm;

private:
	// noncopyable, nonassignable
	MessageProcessor(MessageProcessor const&);
	MessageProcessor& operator=(MessageProcessor const&);
};

/*
struct ProcessorShell : public GC::Managed<>, public std::binary_function< MessageType, JabberMessage, LmHandlerResult > {
public:
	ProcessorShell(MessageProcessor* mpm) : _mpm(mpm) { }

	LmHandlerResult operator()(MessageType type, JabberMessage msg)
	{
		return (*this->_mpm)(type, msg);
	}
private:
	MessageProcessor* _mpm;
};
*/

void initialize_received_message_processors(SessionManager* sm, MessageProcessorMap& mpm);

void destroy_received_message_processors(MessageProcessorMap& mpm);

}

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
