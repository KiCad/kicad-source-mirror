///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CONSTRAINT_LIST_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CONSTRAINT_LIST_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxListCtrl* m_list;
		wxButton* m_deleteButton;
		wxButton* m_closeButton;

		// Virtual event handlers, override them in your derived class
		virtual void onRowActivated( wxListEvent& event ) { event.Skip(); }
		virtual void onDelete( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_CONSTRAINT_LIST_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Geometric Constraints"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 560,360 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_CONSTRAINT_LIST_BASE();

};

