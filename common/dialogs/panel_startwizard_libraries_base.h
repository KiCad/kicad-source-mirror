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
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_STARTWIZARD_LIBRARIES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_STARTWIZARD_LIBRARIES_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_stIntro;
		wxStaticText* m_stRequiredTablesLabel;
		wxStaticText* m_stRequiredTables;
		wxStaticText* m_stQuery;
		wxRadioButton* m_rbDefaultTables;
		wxRadioButton* m_rbImport;
		wxRadioButton* m_rbBlankTables;

	public:

		PANEL_STARTWIZARD_LIBRARIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 550,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_STARTWIZARD_LIBRARIES_BASE();

};

