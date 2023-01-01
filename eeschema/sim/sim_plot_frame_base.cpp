///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "tool/action_toolbar.h"

#include "sim_plot_frame_base.h"

///////////////////////////////////////////////////////////////////////////

SIM_PLOT_FRAME_BASE::SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : KIWAY_PLAYER( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_sizerMain = new wxBoxSizer( wxVERTICAL );

	m_toolBar = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE|wxAUI_TB_HORZ_LAYOUT|wxAUI_TB_PLAIN_BACKGROUND );
	m_toolBar->Realize();

	m_sizerMain->Add( m_toolBar, 0, wxEXPAND, 5 );

	m_splitterLeftRight = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxBORDER_NONE );
	m_splitterLeftRight->SetSashGravity( 0.7 );
	m_splitterLeftRight->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterLeftRightOnIdle ), NULL, this );
	m_splitterLeftRight->SetMinimumPaneSize( 50 );

	m_panelLeft = new wxPanel( m_splitterLeftRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLeft->SetMinSize( wxSize( 300,-1 ) );

	m_sizer11 = new wxBoxSizer( wxVERTICAL );

	m_splitterPlotAndConsole = new wxSplitterWindow( m_panelLeft, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitterPlotAndConsole->SetSashGravity( 0.8 );
	m_splitterPlotAndConsole->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterPlotAndConsoleOnIdle ), NULL, this );
	m_splitterPlotAndConsole->SetMinimumPaneSize( 50 );

	m_plotPanel = new wxPanel( m_splitterPlotAndConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_plotPanel->SetMinSize( wxSize( -1,200 ) );

	m_sizerPlot = new wxBoxSizer( wxHORIZONTAL );

	m_workbook = new SIM_WORKBOOK( m_plotPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_CLOSE_ON_ALL_TABS|wxAUI_NB_MIDDLE_CLICK_CLOSE|wxAUI_NB_TAB_MOVE|wxAUI_NB_TOP );
	m_workbook->SetMinSize( wxSize( 200,-1 ) );


	m_sizerPlot->Add( m_workbook, 1, wxEXPAND, 5 );


	m_plotPanel->SetSizer( m_sizerPlot );
	m_plotPanel->Layout();
	m_sizerPlot->Fit( m_plotPanel );
	m_panelConsole = new wxPanel( m_splitterPlotAndConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelConsole->SetMinSize( wxSize( -1,100 ) );

	m_sizerConsole = new wxBoxSizer( wxVERTICAL );

	m_simConsole = new wxTextCtrl( m_panelConsole, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	m_simConsole->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	m_simConsole->SetMinSize( wxSize( -1,100 ) );

	m_sizerConsole->Add( m_simConsole, 1, wxEXPAND, 5 );


	m_panelConsole->SetSizer( m_sizerConsole );
	m_panelConsole->Layout();
	m_sizerConsole->Fit( m_panelConsole );
	m_splitterPlotAndConsole->SplitHorizontally( m_plotPanel, m_panelConsole, 500 );
	m_sizer11->Add( m_splitterPlotAndConsole, 1, wxEXPAND, 5 );


	m_panelLeft->SetSizer( m_sizer11 );
	m_panelLeft->Layout();
	m_sizer11->Fit( m_panelLeft );
	m_sidePanel = new wxPanel( m_splitterLeftRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sidePanel->SetMinSize( wxSize( 200,-1 ) );

	m_sideSizer = new wxBoxSizer( wxVERTICAL );

	m_splitterSignals = new wxSplitterWindow( m_sidePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE );
	m_splitterSignals->SetSashGravity( 0.33 );
	m_splitterSignals->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterSignalsOnIdle ), NULL, this );
	m_splitterSignals->SetMinimumPaneSize( 20 );

	m_panelSignals = new wxPanel( m_splitterSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelSignals->SetMinSize( wxSize( -1,100 ) );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	m_staticTextSignals = new wxStaticText( m_panelSignals, wxID_ANY, _("Signals"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextSignals->Wrap( -1 );
	bSizer10->Add( m_staticTextSignals, 0, wxTOP|wxRIGHT, 5 );

	m_signals = new wxListView( m_panelSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer10->Add( m_signals, 1, wxEXPAND, 5 );


	m_panelSignals->SetSizer( bSizer10 );
	m_panelSignals->Layout();
	bSizer10->Fit( m_panelSignals );
	m_panelCursorsAndTune = new wxPanel( m_splitterSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelCursorsAndTune->SetMinSize( wxSize( -1,300 ) );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	m_splitterTuneValues = new wxSplitterWindow( m_panelCursorsAndTune, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH|wxSP_LIVE_UPDATE|wxBORDER_NONE );
	m_splitterTuneValues->SetSashGravity( 0.5 );
	m_splitterTuneValues->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterTuneValuesOnIdle ), NULL, this );
	m_splitterTuneValues->SetMinimumPaneSize( 20 );

	m_panelCursors = new wxPanel( m_splitterTuneValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelCursors->SetMinSize( wxSize( -1,100 ) );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxVERTICAL );

	m_staticTextCursors = new wxStaticText( m_panelCursors, wxID_ANY, _("Cursors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCursors->Wrap( -1 );
	bSizer12->Add( m_staticTextCursors, 0, wxTOP|wxRIGHT, 5 );

	m_cursors = new wxListCtrl( m_panelCursors, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer12->Add( m_cursors, 1, wxEXPAND, 5 );


	m_panelCursors->SetSizer( bSizer12 );
	m_panelCursors->Layout();
	bSizer12->Fit( m_panelCursors );
	m_tunePanel = new wxPanel( m_splitterTuneValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_tunePanel->SetMinSize( wxSize( -1,100 ) );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_staticTextTune = new wxStaticText( m_tunePanel, wxID_ANY, _("Tune"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTune->Wrap( -1 );
	bSizer13->Add( m_staticTextTune, 0, wxTOP|wxRIGHT, 5 );

	m_tuneSizer = new wxBoxSizer( wxHORIZONTAL );


	bSizer13->Add( m_tuneSizer, 1, wxEXPAND, 5 );


	m_tunePanel->SetSizer( bSizer13 );
	m_tunePanel->Layout();
	bSizer13->Fit( m_tunePanel );
	m_splitterTuneValues->SplitHorizontally( m_panelCursors, m_tunePanel, 0 );
	bSizer9->Add( m_splitterTuneValues, 1, wxEXPAND, 5 );


	m_panelCursorsAndTune->SetSizer( bSizer9 );
	m_panelCursorsAndTune->Layout();
	bSizer9->Fit( m_panelCursorsAndTune );
	m_splitterSignals->SplitHorizontally( m_panelSignals, m_panelCursorsAndTune, 0 );
	m_sideSizer->Add( m_splitterSignals, 1, wxEXPAND, 5 );


	m_sidePanel->SetSizer( m_sideSizer );
	m_sidePanel->Layout();
	m_sideSizer->Fit( m_sidePanel );
	m_splitterLeftRight->SplitVertically( m_panelLeft, m_sidePanel, 700 );
	m_sizerMain->Add( m_splitterLeftRight, 1, wxEXPAND, 5 );


	this->SetSizer( m_sizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_workbook->Connect( wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotDragged ), NULL, this );
	m_workbook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotChanged ), NULL, this );
	m_workbook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotClose ), NULL, this );
	m_workbook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotClosed ), NULL, this );
	m_signals->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SIM_PLOT_FRAME_BASE::onSignalDblClick ), NULL, this );
	m_signals->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( SIM_PLOT_FRAME_BASE::onSignalRClick ), NULL, this );
	m_cursors->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( SIM_PLOT_FRAME_BASE::onCursorRClick ), NULL, this );
}

SIM_PLOT_FRAME_BASE::~SIM_PLOT_FRAME_BASE()
{
	// Disconnect Events
	m_workbook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_END_DRAG, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotDragged ), NULL, this );
	m_workbook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotChanged ), NULL, this );
	m_workbook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotClose ), NULL, this );
	m_workbook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotClosed ), NULL, this );
	m_signals->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SIM_PLOT_FRAME_BASE::onSignalDblClick ), NULL, this );
	m_signals->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( SIM_PLOT_FRAME_BASE::onSignalRClick ), NULL, this );
	m_cursors->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( SIM_PLOT_FRAME_BASE::onCursorRClick ), NULL, this );

}
