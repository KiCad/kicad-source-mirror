///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COLOR_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COLOR_SETTINGS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxStaticText* m_staticText9;
		wxChoice* m_cbTheme;
		wxButton* m_btnOpenFolder;
		wxStaticLine* m_staticline2;
		wxBoxSizer* m_colorsMainSizer;
		wxScrolledWindow* m_colorsListWindow;
		wxFlexGridSizer* m_colorsGridSizer;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnThemeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLeftDownTheme( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnOverrideItemColorsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnOpenThemeFolderClicked( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxCheckBox* m_optOverrideColors;

		PANEL_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 826,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_COLOR_SETTINGS_BASE();

};

