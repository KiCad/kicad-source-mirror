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
#include "calculator_panels/panel_regulator.h"
#include "calculator_panels/panel_attenuators.h"
#include "calculator_panels/panel_eserie.h"
#include "calculator_panels/panel_color_code.h"
#include "calculator_panels/panel_transline.h"
#include "calculator_panels/panel_via_size.h"
#include "calculator_panels/panel_track_width.h"
#include "calculator_panels/panel_electrical_spacing.h"
#include "calculator_panels/panel_board_class.h"
#include "kiway_player.h"
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PCB_CALCULATOR_FRAME_BASE
///////////////////////////////////////////////////////////////////////////////
class PCB_CALCULATOR_FRAME_BASE : public KIWAY_PLAYER
{
	private:

	protected:
		wxMenuBar* m_menubar;
		wxNotebook* m_Notebook;
		PANEL_REGULATOR* m_panelRegulators;
		PANEL_ATTENUATORS* m_panelAttenuators;
		PANEL_E_SERIE* m_panelESeries;
		PANEL_COLOR_CODE* m_panelColorCode;
		PANEL_TRANSLINE* m_panelTransline;
		PANEL_VIA_SIZE* m_panelViaSize;
		PANEL_TRACK_WIDTH* m_panelTrackWidth;
		PANEL_ELECTRICAL_SPACING* m_panelElectricalSpacing;
		PANEL_BOARD_CLASS* m_panelBoardClass;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClosePcbCalc( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		PCB_CALCULATOR_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("PCB Calculator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 646,361 ), long style = wxDEFAULT_FRAME_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxTAB_TRAVERSAL, const wxString& name = wxT("pcb_calculator") );

		~PCB_CALCULATOR_FRAME_BASE();

};

