///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_HTML_REPORT_BOX;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/html/htmlwin.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ASSIGN_NETCLASS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ASSIGN_NETCLASS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* stPatternLabel;
		wxTextCtrl* m_patternCtrl;
		wxStaticText* stNetclassLabel;
		wxComboBox* m_netclassCtrl;
		WX_HTML_REPORT_BOX* m_matchingNets;
		wxStaticText* m_info;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onPatternText( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_ASSIGN_NETCLASS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Add Netclass Assignment"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_ASSIGN_NETCLASS_BASE();

};

