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
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_SYMBOL_PARITY_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_SYMBOL_PARITY_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText26;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_missingFields;
		wxCheckBox* m_extraFields;
		wxCheckBox* m_fieldTextOpt;
		wxCheckBox* m_fieldVisibilitiesOpt;
		wxCheckBox* m_fieldStyleOpt;
		wxCheckBox* m_fieldPositionsOpt;
		wxStaticText* m_staticText27;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_pinVisibilitiesOpt;
		wxCheckBox* m_altPinFunctionsOpt;
		wxStaticText* m_staticText28;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_excludeFromBoardOpt;
		wxCheckBox* m_DNPOpt;
		wxCheckBox* m_excludeFromBOMOpt;
		wxCheckBox* m_excludeFromPosFilesOpt;

	public:

		PANEL_SETUP_SYMBOL_PARITY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_SYMBOL_PARITY_BASE();

};

