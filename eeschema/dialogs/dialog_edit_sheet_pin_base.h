///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EDIT_SHEET_PIN_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EDIT_SHEET_PIN_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textName;
		wxStaticText* m_heightLabel;
		wxTextCtrl* m_heightCtrl;
		wxStaticText* m_heightUnits;
		wxStaticText* m_widthLabel;
		wxTextCtrl* m_widthCtrl;
		wxStaticText* m_widthUnits;
		wxStaticText* m_staticText3;
		wxChoice* m_choiceConnectionType;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onOKButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EDIT_SHEET_PIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Sheet Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_EDIT_SHEET_PIN_BASE();

};

