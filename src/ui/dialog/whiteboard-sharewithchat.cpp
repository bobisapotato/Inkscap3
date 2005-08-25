/**
 * Whiteboard share with chatroom dialog
 *
 * Authors:
 * David Yip <yipdw@rose-hulman.edu>
 * Jason Segal, Jonas Collaros, Stephen Montgomery, Brandi Soggs, Matthew Weinstock (original C/Gtk version)
 *
 * Copyright (c) 2004-2005 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>

#include <sigc++/sigc++.h>
#include <gtk/gtkdialog.h>

#include "message-context.h"
#include "inkscape.h"
#include "desktop.h"

#include "jabber_whiteboard/typedefs.h"
#include "jabber_whiteboard/session-manager.h"
#include "jabber_whiteboard/buddy-list-manager.h"

#include "jabber_whiteboard/session-file-selector.h"

#include "ui/dialog/whiteboard-sharewithchat.h"

#include "util/ucompose.hpp"

namespace Inkscape {

namespace UI {

namespace Dialog {

WhiteboardShareWithChatroomDialog* 
WhiteboardShareWithChatroomDialog::create()
{
	return new WhiteboardShareWithChatroomDialogImpl();
}

WhiteboardShareWithChatroomDialogImpl::WhiteboardShareWithChatroomDialogImpl() 
{
	this->setSessionManager();
	this->_construct();
	this->get_vbox()->show_all_children();
}

WhiteboardShareWithChatroomDialogImpl::~WhiteboardShareWithChatroomDialogImpl()
{

}

void
WhiteboardShareWithChatroomDialogImpl::setSessionManager()
{
	this->_desktop = SP_ACTIVE_DESKTOP;
	this->_sm = SP_ACTIVE_DESKTOP->whiteboard_session_manager();

	g_log(NULL, G_LOG_LEVEL_DEBUG, "desktop=%p _sm=%p", this->_desktop, this->_sm);
}


void
WhiteboardShareWithChatroomDialogImpl::_construct()
{
	Gtk::VBox* main = this->get_vbox();

	// Construct labels
	this->_labels[0].set_markup_with_mnemonic(_("Chatroom _name:"));
	this->_labels[1].set_markup_with_mnemonic(_("Chatroom _server:"));
	this->_labels[2].set_markup_with_mnemonic(_("Chatroom _password:"));
	this->_labels[3].set_markup_with_mnemonic(_("Chatroom _handle:"));

	this->_labels[0].set_mnemonic_widget(this->_roomname);
	this->_labels[1].set_mnemonic_widget(this->_confserver);
	this->_labels[2].set_mnemonic_widget(this->_roompass);
	this->_labels[3].set_mnemonic_widget(this->_handle);

	// Pack labels and entry boxes
	this->_roomnamebox.pack_start(this->_labels[0]);
	this->_roomnamebox.pack_start(this->_roomname);

	this->_confserverbox.pack_start(this->_labels[1]);
	this->_confserverbox.pack_start(this->_confserver);

	this->_roompassbox.pack_start(this->_labels[2]);
	this->_roompassbox.pack_start(this->_roompass);

	this->_handlebox.pack_start(this->_labels[3]);
	this->_handlebox.pack_start(this->_handle);

	// Button setup and callback registration
	this->_share.set_label(_("Connect to chatroom"));
	this->_cancel.set_label(_("Cancel"));
	this->_share.set_use_underline(true);
	this->_cancel.set_use_underline(true);

	this->_share.signal_clicked().connect(sigc::bind< 0 >(sigc::mem_fun(*this, &WhiteboardShareWithChatroomDialogImpl::_respCallback), WhiteboardShareWithChatroomDialogImpl::SHARE));
	this->_cancel.signal_clicked().connect(sigc::bind< 0 >(sigc::mem_fun(*this, &WhiteboardShareWithChatroomDialogImpl::_respCallback), WhiteboardShareWithChatroomDialogImpl::CANCEL));

	// Pack buttons
	this->_buttonsbox.pack_start(this->_cancel);
	this->_buttonsbox.pack_start(this->_share);

	// Set default values
	Glib::ustring jid = lm_connection_get_jid(this->_sm->session_data->connection);
	Glib::ustring nick = jid.substr(0, jid.find_first_of('@'));
	this->_handle.set_text(nick);
	this->_roomname.set_text("inkboard");

	// Pack into main box
	main->pack_start(this->_roomnamebox);
	main->pack_start(this->_confserverbox);
	main->pack_start(this->_roompassbox);
	main->pack_start(this->_handlebox);
	main->pack_end(this->_buttonsbox);
}

void
WhiteboardShareWithChatroomDialogImpl::_respCallback(int resp)
{
	switch (resp) {
		case SHARE:
		{
			Glib::ustring chatroom, server, handle, password;
			chatroom = this->_roomname.get_text();
			server = this->_confserver.get_text();
			password = this->_roompass.get_text();
			handle = this->_handle.get_text();

			Glib::ustring msg = String::ucompose(_("Synchronizing with chatroom <b>%1@%2</b> using the handle <b>%3</b>"), chatroom, server, handle);

			this->_desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, msg.data());

			this->_desktop->whiteboard_session_manager()->sendRequestToChatroom(server, chatroom, handle, password);
		}
		case CANCEL:
		default:
			this->hide();
			break;
	}
}

}

}

}
