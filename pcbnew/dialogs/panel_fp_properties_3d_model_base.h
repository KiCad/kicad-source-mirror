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
class COLOR_SWATCH;
class STD_BITMAP_BUTTON;
class WX_GRID;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FP_PROPERTIES_3D_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_PROPERTIES_3D_MODEL_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* bSizerMain3D;
		wxSplitterWindow* m_splitter1;
		wxPanel* m_upperPanel;
		WX_GRID* m_modelsGrid;
		STD_BITMAP_BUTTON* m_button3DShapeAdd;
		STD_BITMAP_BUTTON* m_button3DShapeBrowse;
		STD_BITMAP_BUTTON* m_button3DShapeRemove;
		wxButton* m_buttonConfig3DPaths;
		wxCheckBox* m_enableExtrusionCheckbox;
		wxCheckBox* m_showExtrusionCheckbox;
		wxStaticText* m_componentHeightLabel;
		wxTextCtrl* m_componentHeightCtrl;
		wxStaticText* m_componentHeightUnits;
		wxStaticText* m_standoffHeightLabel;
		wxTextCtrl* m_standoffHeightCtrl;
		wxStaticText* m_standoffHeightUnits;
		wxStaticText* m_extrusionLayerLabel;
		wxChoice* m_extrusionLayerChoice;
		wxStaticText* m_extrusionColorLabel;
		COLOR_SWATCH* m_extrusionColorSwatch;
		wxStaticText* m_extrusionMaterialLabel;
		wxChoice* m_extrusionMaterialChoice;
		wxButton* m_buttonExportExtruded;
		wxPanel* m_lowerPanel;
		wxBoxSizer* m_LowerSizer3D;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void On3DModelCellChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void On3DModelSelected( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAdd3DRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAdd3DModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemove3DModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void Cfg3DPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEnableExtrusion( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExportExtrudedModel( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_FP_PROPERTIES_3D_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 778,420 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_FP_PROPERTIES_3D_MODEL_BASE();

		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 300 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_FP_PROPERTIES_3D_MODEL_BASE::m_splitter1OnIdle ), NULL, this );
		}

};

