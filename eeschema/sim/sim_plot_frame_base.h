///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class ACTION_TOOLBAR;
class wxListView;

#include "sim_workbook.h"
#include "kiway_player.h"
#include <wx/gdicmn.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/aui/auibook.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
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
		wxBoxSizer* m_sizerMain;
		ACTION_TOOLBAR* m_toolBar;
		wxSplitterWindow* m_splitterLeftRight;
		wxPanel* m_panelLeft;
		wxBoxSizer* m_sizer11;
		wxSplitterWindow* m_splitterPlotAndConsole;
		wxPanel* m_plotPanel;
		wxBoxSizer* m_sizerPlot;
		SIM_WORKBOOK* m_workbook;
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

		// Virtual event handlers, override them in your derived class
		virtual void onPlotDragged( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onPlotChanged( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onPlotClose( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onPlotClosed( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void onSignalDblClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSignalRClick( wxListEvent& event ) { event.Skip(); }
		virtual void onCursorRClick( wxListEvent& event ) { event.Skip(); }


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

