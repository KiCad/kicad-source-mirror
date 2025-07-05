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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_STARTWIZARD_PRIVACY_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_STARTWIZARD_PRIVACY_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_stIntro;
		wxCheckBox* m_cbAutoUpdateKiCad;
		wxCheckBox* m_cbAutoUpdatePCM;
		wxStaticBoxSizer* m_sizerDataCollection;
		wxStaticText* m_stIntroDataCollection;
		wxCheckBox* m_cbDataCollection;

	public:

		PANEL_STARTWIZARD_PRIVACY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_STARTWIZARD_PRIVACY_BASE();

};

