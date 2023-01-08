///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NEW_DATAITEM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NEW_DATAITEM_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextPosY;
		wxTextCtrl* m_textCtrlPosY;
		wxStaticText* m_units1;
		wxStaticText* m_staticTextPosX;
		wxTextCtrl* m_textCtrlPosX;
		wxStaticText* m_units11;
		wxStaticText* m_staticTextOrgPos;
		wxChoice* m_choiceCornerPos;
		wxStaticLine* m_staticline2;
		wxBoxSizer* m_SizerEndPosition;
		wxStaticText* m_staticTextEndX;
		wxTextCtrl* m_textCtrlEndX;
		wxStaticText* m_units3;
		wxStaticText* m_staticTextEndY;
		wxTextCtrl* m_textCtrlEndY;
		wxStaticText* m_units4;
		wxStaticText* m_staticTextOrgPos1;
		wxChoice* m_choiceCornerEnd;
		wxBoxSizer* m_SizerText;
		wxStaticText* m_staticTextTitle;
		wxTextCtrl* m_textCtrlText;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_NEW_DATAITEM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("New Item"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_NEW_DATAITEM_BASE();

};

