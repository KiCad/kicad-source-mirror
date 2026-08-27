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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_2581_BOM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_2581_BOM_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_lblBomRev;
		wxTextCtrl* m_textBomRev;
		wxStaticText* m_lblOEM;
		wxChoice* m_oemRef;
		wxStaticText* m_staticText6;
		wxChoice* m_choiceMPN;
		wxStaticText* m_staticText7;
		wxChoice* m_choiceMfg;
		wxStaticText* m_staticText8;
		wxChoice* m_choiceDistPN;
		wxStaticText* m_staticText9;
		wxTextCtrl* m_textDistributor;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onMfgPNChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDistPNChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOKClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_2581_BOM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("BOM Fields"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_2581_BOM_BASE();

};

