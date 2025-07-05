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
class STD_BITMAP_BUTTON;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobut.h>
#include <wx/combobox.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>

#include "kicommon.h"

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_STARTWIZARD_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class KICOMMON_API PANEL_STARTWIZARD_SETTINGS_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_sizer;
		wxStaticText* m_staticText2;
		wxRadioButton* m_btnPrevVer;
		wxComboBox* m_cbPath;
		STD_BITMAP_BUTTON* m_btnCustomPath;
		wxStaticText* m_lblPathError;
		wxRadioButton* m_btnUseDefaults;

		// Virtual event handlers, override them in your derived class
		virtual void OnPrevVerSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPathChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPathDefocused( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnChoosePath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDefaultSelected( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_STARTWIZARD_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_STARTWIZARD_SETTINGS_BASE();

};

