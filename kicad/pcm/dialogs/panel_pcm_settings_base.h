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
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCM_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCM_SETTINGS_BASE : public wxPanel
{
	private:

	protected:
		wxCheckBox* m_updateCheck;
		wxCheckBox* m_libAutoAdd;
		wxCheckBox* m_libAutoRemove;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_libPrefix;
		wxStaticText* m_libHelp;

	public:

		PANEL_PCM_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PCM_SETTINGS_BASE();

};

