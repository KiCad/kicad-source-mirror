///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_CHOOSE_PROJECT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_CHOOSE_PROJECT_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerMain;
		wxStaticText* m_titleText;
		wxListCtrl* m_listCtrl;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void onItemActivated( wxListEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_CHOOSE_PROJECT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Choose Project to Import"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxRESIZE_BORDER );

		~DIALOG_IMPORT_CHOOSE_PROJECT_BASE();

};

