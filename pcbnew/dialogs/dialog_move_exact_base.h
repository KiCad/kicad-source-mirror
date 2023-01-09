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
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MOVE_EXACT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MOVE_EXACT_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bMainSizer;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xEntry;
		wxStaticText* m_xUnit;
		wxButton* m_clearX;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yEntry;
		wxStaticText* m_yUnit;
		wxButton* m_clearY;
		wxStaticText* m_rotLabel;
		wxTextCtrl* m_rotEntry;
		wxStaticText* m_rotUnit;
		wxButton* m_clearRot;
		wxChoice* m_anchorOptions;
		wxCheckBox* m_polarCoords;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnTextFocusLost( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnTextChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClear( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPolarChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_MOVE_EXACT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Move Item"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MOVE_EXACT_BASE();

};

