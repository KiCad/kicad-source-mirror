///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ENUM_PADS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ENUM_PADS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_lblInfo;
		wxStaticText* m_lblPadPrefix;
		wxTextCtrl* m_padPrefix;
		wxStaticText* m_lblPadStartNum;
		wxSpinCtrl* m_padStartNum;
		wxStaticText* m_lblPadNumStep;
		wxSpinCtrl* m_padNumStep;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

	public:

		DIALOG_ENUM_PADS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pad Enumeration Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_ENUM_PADS_BASE();

};

