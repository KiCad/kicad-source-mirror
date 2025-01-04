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
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PACKAGES_AND_UPDATES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PACKAGES_AND_UPDATES_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_generalLabel;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_cbKicadUpdate;
		wxStaticText* m_pcmLabel;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_cbPcmUpdate;
		wxStaticText* m_staticText4;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_libAutoAdd;
		wxCheckBox* m_libAutoRemove;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_libPrefix;

	public:

		PANEL_PACKAGES_AND_UPDATES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PACKAGES_AND_UPDATES_BASE();

};

