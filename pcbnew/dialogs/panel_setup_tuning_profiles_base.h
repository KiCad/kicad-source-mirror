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

#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_TUNING_PROFILES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_TUNING_PROFILES_BASE : public wxPanel
{
	private:

	protected:
		wxNotebook* m_tuningProfiles;
		STD_BITMAP_BUTTON* m_addTuningProfileButton;
		STD_BITMAP_BUTTON* m_removeTuningProfileButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnAddTuningProfileClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTuningProfileClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_TUNING_PROFILES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 719,506 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_TUNING_PROFILES_BASE();

};

