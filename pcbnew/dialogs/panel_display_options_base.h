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

#include "widgets/resettable_panel.h"
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_DISPLAY_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxBoxSizer* m_galOptionsSizer;
		wxStaticText* m_padsLabel;
		wxStaticLine* m_staticlinePads;
		wxCheckBox* m_OptUseViaColorForNormalTHPadstacks;
		wxStaticText* m_clearanceLabel;
		wxStaticLine* m_staticline2;
		wxStaticText* m_trackClearancesLabel;
		wxChoice* m_OptDisplayTracksClearance;
		wxCheckBox* m_OptDisplayPadClearence;
		wxSimplebook* m_optionsBook;
		wxStaticText* m_layerNamesLabel;
		WX_GRID* m_layerNameitemsGrid;
		STD_BITMAP_BUTTON* m_bpAddLayer;
		STD_BITMAP_BUTTON* m_bpDeleteLayer;
		wxStaticText* m_annotationsLabel;
		wxStaticLine* m_staticline1;
		wxStaticText* m_netNamesLabel;
		wxChoice* m_ShowNetNamesOption;
		wxCheckBox* m_OptDisplayPadNumber;
		wxStaticText* m_staticText4;
		wxStaticLine* m_staticline4;
		wxCheckBox* m_checkForceShowFieldsWhenFPSelected;
		wxStaticText* m_crossProbingLabel;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkCrossProbeOnSelection;
		wxCheckBox* m_checkCrossProbeCenter;
		wxCheckBox* m_checkCrossProbeZoom;
		wxCheckBox* m_checkCrossProbeAutoHighlight;
		wxCheckBox* m_checkCrossProbeFlash;
		wxCheckBox* m_live3Drefresh;

		// Virtual event handlers, override them in your derived class
		virtual void onLayerChange( wxGridEvent& event ) { event.Skip(); }
		virtual void OnGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddLayerItem( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteLayerItem( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_DISPLAY_OPTIONS_BASE();

};

