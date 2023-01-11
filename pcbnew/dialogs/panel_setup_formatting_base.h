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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_FORMATTING_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_FORMATTING_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* dashLengthLabel;
		wxTextCtrl* m_dashLengthCtrl;
		wxStaticText* gapLengthLabel;
		wxTextCtrl* m_gapLengthCtrl;
		wxStaticText* m_dashedLineHelp;

	public:

		PANEL_SETUP_FORMATTING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_FORMATTING_BASE();

};

