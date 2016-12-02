///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SIM_PLOT_FRAME_BASE_H__
#define __SIM_PLOT_FRAME_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class KIWAY_PLAYER;
class wxListView;

#include "kiway_player.h"
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/toolbar.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/aui/auibook.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/statbox.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


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
		wxPanel* m_welcomePanel;
		wxBoxSizer* m_sizer8;
		wxStaticText* m_staticText2;
		wxPanel* m_panelConsole;
		wxBoxSizer* m_sizerConsole;
		wxTextCtrl* m_simConsole;
		wxPanel* m_sidePanel;
		wxBoxSizer* m_sideSizer;
		wxSplitterWindow* m_splitterSignals;
		wxPanel* m_panelSignals;
		wxListView* m_signals;
		wxPanel* m_panelCursorsAndTune;
		wxSplitterWindow* m_splitterTuneValues;
		wxPanel* m_panelCursors;
		wxListCtrl* m_cursors;
		wxPanel* m_tunePanel;
		wxStaticBoxSizer* m_tuneSizerStb;
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
		virtual void onPlotChanged( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onPlotClose( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onSignalDblClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSignalRClick( wxListEvent& event ) { event.Skip(); }
		
	
	public:
		
		SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Spice Simulator"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 883,594 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL, const wxString& name = wxT("SIM_PLOT_FRAME") );
		
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

#endif //__SIM_PLOT_FRAME_BASE_H__
