///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include "kiway_player.h"
#include <wx/artprov.h>
#include <wx/aui/auibook.h>
#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/font.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/icon.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>
#include <wx/xrc/xmlres.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SAVE_AS_IMAGE 1000
#define ID_SAVE_AS_CSV 1001
#define ID_MENU_RUN_SIM 1002
#define ID_MENU_ADD_SIGNAL 1003
#define ID_MENU_PROBE_SIGNALS 1004
#define ID_MENU_TUNE_SIGNALS 1005
#define ID_MENU_SHOW_NETLIST 1006
#define ID_MENU_SET_SIMUL 1007
#define ID_MENU_SHOW_GRID 1008
#define ID_MENU_SHOW_LEGEND 1009
#define ID_MENU_DOTTED 1010
#define ID_MENU_WHITE_BG 1011

///////////////////////////////////////////////////////////////////////////////
/// Class SIM_PLOT_FRAME_BASE
///////////////////////////////////////////////////////////////////////////////
class SIM_PLOT_FRAME_BASE : public KIWAY_PLAYER
{
	private:

	protected:
		wxMenuBar* m_mainMenu;
		wxMenu* m_fileMenu;
		wxMenu* m_simulationMenu;
		wxMenuItem* m_runSimulation;
		wxMenuItem* m_addSignals;
		wxMenuItem* m_probeSignals;
		wxMenuItem* m_tuneValue;
		wxMenuItem* m_showNetlist;
		wxMenuItem* m_settings;
		wxMenu* m_viewMenu;
		wxBoxSizer* m_sizerMain;
		wxToolBar* m_toolBar;
		wxSplitterWindow* m_splitterLeftRight;
		wxPanel* m_panelLeft;
		wxBoxSizer* m_sizer11;
		wxSplitterWindow* m_splitterPlotAndConsole;
		wxPanel* m_plotPanel;
		wxBoxSizer* m_sizerPlot;
		wxAuiNotebook* m_plotNotebook;
		wxPanel* m_panelConsole;
		wxBoxSizer* m_sizerConsole;
		wxTextCtrl* m_simConsole;
		wxPanel* m_sidePanel;
		wxBoxSizer* m_sideSizer;
		wxSplitterWindow* m_splitterSignals;
		wxPanel* m_panelSignals;
		wxStaticText* m_staticTextSignals;
		wxListView* m_signals;
		wxPanel* m_panelCursorsAndTune;
		wxSplitterWindow* m_splitterTuneValues;
		wxPanel* m_panelCursors;
		wxStaticText* m_staticTextCursors;
		wxListCtrl* m_cursors;
		wxPanel* m_tunePanel;
		wxStaticText* m_staticTextTune;
		wxBoxSizer* m_tuneSizer;

		// Virtual event handlers, overide them in your derived class
		virtual void menuNewPlot( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuOpenWorkbook( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuSaveWorkbook( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuSaveImage( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuSaveCsv( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuZoomIn( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuZoomOut( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuZoomFit( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowGrid( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowGridUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void menuShowLegend( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowLegendUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void menuShowDotted( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowDottedUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void menuWhiteBackground( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowWhiteBackgroundUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onPlotChanged( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onPlotClose( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onSignalDblClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSignalRClick( wxListEvent& event ) { event.Skip(); }


	public:

		SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Spice Simulator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 564,531 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL, const wxString& name = wxT("SIM_PLOT_FRAME") );

		~SIM_PLOT_FRAME_BASE();

		void m_splitterLeftRightOnIdle( wxIdleEvent& )
		{
			m_splitterLeftRight->SetSashPosition( 700 );
			m_splitterLeftRight->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterLeftRightOnIdle ), NULL, this );
		}

		void m_splitterPlotAndConsoleOnIdle( wxIdleEvent& )
		{
			m_splitterPlotAndConsole->SetSashPosition( 500 );
			m_splitterPlotAndConsole->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterPlotAndConsoleOnIdle ), NULL, this );
		}

		void m_splitterSignalsOnIdle( wxIdleEvent& )
		{
			m_splitterSignals->SetSashPosition( 0 );
			m_splitterSignals->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterSignalsOnIdle ), NULL, this );
		}

		void m_splitterTuneValuesOnIdle( wxIdleEvent& )
		{
			m_splitterTuneValues->SetSashPosition( 0 );
			m_splitterTuneValues->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterTuneValuesOnIdle ), NULL, this );
		}

};

