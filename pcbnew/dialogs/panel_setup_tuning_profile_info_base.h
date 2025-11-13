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
class WX_GRID;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_TUNING_PROFILE_INFO_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_TUNING_PROFILE_INFO_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_nameLabel;
		wxTextCtrl* m_name;
		wxStaticText* m_typeLabel;
		wxChoice* m_type;
		wxStaticText* m_targetImpedanceLabel;
		wxTextCtrl* m_targetImpedance;
		wxStaticText* m_ohmsLabel;
		wxCheckBox* m_enableDelayTuning;
		wxStaticLine* m_staticline1;
		wxSplitterWindow* m_splitter1;
		wxPanel* m_panel3;
		WX_GRID* m_trackPropagationGrid;
		STD_BITMAP_BUTTON* m_addTrackPropogationLayer;
		STD_BITMAP_BUTTON* m_deleteTrackPropogationLayer;
		wxPanel* m_panel4;
		wxStaticText* m_viaPropagationSpeedLabel;
		wxTextCtrl* m_viaPropagationSpeed;
		wxStaticText* m_viaPropSpeedUnits;
		wxStaticText* m_viaDelayOverridesLabel;
		WX_GRID* m_viaOverrides;
		STD_BITMAP_BUTTON* m_addViaPropagationOverride;
		STD_BITMAP_BUTTON* m_removeViaPropagationOverride;

		// Virtual event handlers, override them in your derived class
		virtual void OnProfileNameChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeProfileType( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddTrackRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveTrackRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddViaOverride( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveViaOverride( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_TUNING_PROFILE_INFO_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 719,506 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_TUNING_PROFILE_INFO_BASE();

		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 0 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_SETUP_TUNING_PROFILE_INFO_BASE::m_splitter1OnIdle ), NULL, this );
		}

};

