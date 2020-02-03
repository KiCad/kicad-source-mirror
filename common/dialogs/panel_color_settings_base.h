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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COLOR_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COLOR_SETTINGS_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxStaticText* m_staticText9;
		wxChoice* m_cbTheme;
		wxButton* m_btnSave;
		wxButton* m_btnNew;
		wxButton* m_btnRename;
		wxButton* m_btnReset;
		wxButton* m_btnOpenFolder;
		wxPanel* m_panelThemeProperties;
		wxBoxSizer* m_sizerThemeProperties;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_txtThemeName;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_txtFilename;
		wxStaticText* m_lblThemePropertiesError;
		wxBoxSizer* m_colorsMainSizer;
		wxScrolledWindow* m_colorsListWindow;
		wxFlexGridSizer* m_colorsGridSizer;

		// Virtual event handlers, overide them in your derived class
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnThemeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnSaveClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnNewClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnRenameClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnResetClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnOpenThemeFolderClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnThemeNameChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilenameChar( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnFilenameChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COLOR_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 826,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_COLOR_SETTINGS_BASE();

};

