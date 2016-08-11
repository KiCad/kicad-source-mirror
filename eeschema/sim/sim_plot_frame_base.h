///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
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
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>
#include <wx/listbox.h>
#include <wx/statbox.h>
#include <wx/listctrl.h>
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
		wxMenu* m_viewMenu;
		wxToolBar* m_toolBar;
		wxSplitterWindow* m_splitterPlot;
		wxPanel* m_panel2;
		wxSplitterWindow* m_splitterConsole;
		wxPanel* m_panel4;
		wxNotebook* m_plotNotebook;
		wxPanel* m_welcomePanel;
		wxStaticText* m_staticText2;
		wxPanel* m_panel5;
		wxTextCtrl* m_simConsole;
		wxPanel* m_panel31;
		wxListBox* m_signals;
		wxListCtrl* m_cursors;
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
		virtual void onPlotChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void onSignalDblClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSignalRClick( wxMouseEvent& event ) { event.Skip(); }
		
	
	public:
		
		SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation Workbook"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1280,900 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL, const wxString& name = wxT("SIM_PLOT_FRAME") );
		
		~SIM_PLOT_FRAME_BASE();
		
		void m_splitterPlotOnIdle( wxIdleEvent& )
		{
			m_splitterPlot->SetSashPosition( 0 );
			m_splitterPlot->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterPlotOnIdle ), NULL, this );
		}
		
		void m_splitterConsoleOnIdle( wxIdleEvent& )
		{
			m_splitterConsole->SetSashPosition( 0 );
			m_splitterConsole->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterConsoleOnIdle ), NULL, this );
		}
	
};

#endif //__SIM_PLOT_FRAME_BASE_H__
