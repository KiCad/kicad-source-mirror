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
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE : public wxPanel
{
	private:

	protected:
		wxFlexGridSizer* m_keepoutRuleSizer;

	public:
		wxCheckBox* m_cbTracksCtrl;
		wxCheckBox* m_cbViasCtrl;
		wxCheckBox* m_cbPadsCtrl;
		wxCheckBox* m_cbCopperPourCtrl;
		wxCheckBox* m_cbFootprintsCtrl;

		PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE();

};

