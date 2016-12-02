///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "sim_plot_frame_base.h"

///////////////////////////////////////////////////////////////////////////

SIM_PLOT_FRAME_BASE::SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : KIWAY_PLAYER( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxSize( 600,500 ), wxDefaultSize );
	
	m_mainMenu = new wxMenuBar( 0 );
	m_fileMenu = new wxMenu();
	wxMenuItem* m_newPlot;
	m_newPlot = new wxMenuItem( m_fileMenu, wxID_NEW, wxString( _("New Plot") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_newPlot );
	
	m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_openWorkbook;
	m_openWorkbook = new wxMenuItem( m_fileMenu, wxID_OPEN, wxString( _("Open Workbook") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_openWorkbook );
	
	wxMenuItem* m_saveWorkbook;
	m_saveWorkbook = new wxMenuItem( m_fileMenu, wxID_SAVE, wxString( _("Save Workbook") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_saveWorkbook );
	
	m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_saveImage;
	m_saveImage = new wxMenuItem( m_fileMenu, wxID_ANY, wxString( _("Save as image") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_saveImage );
	
	wxMenuItem* m_saveCsv;
	m_saveCsv = new wxMenuItem( m_fileMenu, wxID_ANY, wxString( _("Save as .csv file") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_saveCsv );
	
	m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_exitSim;
	m_exitSim = new wxMenuItem( m_fileMenu, wxID_CLOSE, wxString( _("Exit Simulation") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_exitSim );
	
	m_mainMenu->Append( m_fileMenu, _("File") ); 
	
	m_simulationMenu = new wxMenu();
	m_runSimulation = new wxMenuItem( m_simulationMenu, wxID_ANY, wxString( _("Run Simulation") ) , wxEmptyString, wxITEM_NORMAL );
	m_simulationMenu->Append( m_runSimulation );
	
	m_simulationMenu->AppendSeparator();
	
	m_addSignals = new wxMenuItem( m_simulationMenu, wxID_ANY, wxString( _("Add signals...") ) , wxEmptyString, wxITEM_NORMAL );
	m_simulationMenu->Append( m_addSignals );
	
	m_probeSignals = new wxMenuItem( m_simulationMenu, wxID_ANY, wxString( _("Probe from schematics") ) , wxEmptyString, wxITEM_NORMAL );
	m_simulationMenu->Append( m_probeSignals );
	
	m_tuneValue = new wxMenuItem( m_simulationMenu, wxID_ANY, wxString( _("Tune component value") ) , wxEmptyString, wxITEM_NORMAL );
	m_simulationMenu->Append( m_tuneValue );
	
	m_simulationMenu->AppendSeparator();
	
	m_settings = new wxMenuItem( m_simulationMenu, wxID_ANY, wxString( _("Settings...") ) , wxEmptyString, wxITEM_NORMAL );
	m_simulationMenu->Append( m_settings );
	
	m_mainMenu->Append( m_simulationMenu, _("Simulation") ); 
	
	m_viewMenu = new wxMenu();
	wxMenuItem* m_zoomIn;
	m_zoomIn = new wxMenuItem( m_viewMenu, wxID_ZOOM_IN, wxString( _("Zoom In") ) , wxEmptyString, wxITEM_NORMAL );
	m_viewMenu->Append( m_zoomIn );
	
	wxMenuItem* m_zoomOut;
	m_zoomOut = new wxMenuItem( m_viewMenu, wxID_ZOOM_OUT, wxString( _("Zoom Out") ) , wxEmptyString, wxITEM_NORMAL );
	m_viewMenu->Append( m_zoomOut );
	
	wxMenuItem* m_zoomFit;
	m_zoomFit = new wxMenuItem( m_viewMenu, wxID_ZOOM_FIT, wxString( _("Fit on Screen") ) , wxEmptyString, wxITEM_NORMAL );
	m_viewMenu->Append( m_zoomFit );
	
	m_viewMenu->AppendSeparator();
	
	wxMenuItem* m_showGrid;
	m_showGrid = new wxMenuItem( m_viewMenu, wxID_ANY, wxString( _("Show &grid") ) , wxEmptyString, wxITEM_CHECK );
	m_viewMenu->Append( m_showGrid );
	
	wxMenuItem* m_showLegend;
	m_showLegend = new wxMenuItem( m_viewMenu, wxID_ANY, wxString( _("Show &legend") ) , wxEmptyString, wxITEM_CHECK );
	m_viewMenu->Append( m_showLegend );
	
	m_mainMenu->Append( m_viewMenu, _("View") ); 
	
	this->SetMenuBar( m_mainMenu );
	
	m_sizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_toolBar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxTB_TEXT ); 
	m_toolBar->Realize(); 
	
	m_sizerMain->Add( m_toolBar, 0, wxEXPAND, 5 );
	
	m_splitterLeftRight = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterLeftRight->SetSashGravity( 0.7 );
	m_splitterLeftRight->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterLeftRightOnIdle ), NULL, this );
	m_splitterLeftRight->SetMinimumPaneSize( 50 );
	
	m_panelLeft = new wxPanel( m_splitterLeftRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLeft->SetMinSize( wxSize( 300,-1 ) );
	
	m_sizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_splitterPlotAndConsole = new wxSplitterWindow( m_panelLeft, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterPlotAndConsole->SetSashGravity( 0.8 );
	m_splitterPlotAndConsole->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterPlotAndConsoleOnIdle ), NULL, this );
	m_splitterPlotAndConsole->SetMinimumPaneSize( 50 );
	
	m_plotPanel = new wxPanel( m_splitterPlotAndConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_plotPanel->SetMinSize( wxSize( -1,200 ) );
	
	m_sizerPlot = new wxBoxSizer( wxHORIZONTAL );
	
	m_plotNotebook = new wxAuiNotebook( m_plotPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_MIDDLE_CLICK_CLOSE|wxAUI_NB_TAB_MOVE|wxAUI_NB_TAB_SPLIT|wxAUI_NB_TOP );
	m_welcomePanel = new wxPanel( m_plotNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sizer8 = new wxBoxSizer( wxVERTICAL );
	
	
	m_sizer8->Add( 0, 0, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer81->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticText2 = new wxStaticText( m_welcomePanel, wxID_ANY, _("Start the simulation by clicking the Run Simulation button"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	m_staticText2->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );
	m_staticText2->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
	
	bSizer81->Add( m_staticText2, 0, wxALIGN_RIGHT|wxALL|wxEXPAND, 5 );
	
	
	bSizer81->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_sizer8->Add( bSizer81, 0, wxEXPAND, 5 );
	
	
	m_sizer8->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_welcomePanel->SetSizer( m_sizer8 );
	m_welcomePanel->Layout();
	m_sizer8->Fit( m_welcomePanel );
	m_plotNotebook->AddPage( m_welcomePanel, _("a page"), false, wxNullBitmap );
	
	m_sizerPlot->Add( m_plotNotebook, 1, wxEXPAND | wxALL, 5 );
	
	
	m_plotPanel->SetSizer( m_sizerPlot );
	m_plotPanel->Layout();
	m_sizerPlot->Fit( m_plotPanel );
	m_panelConsole = new wxPanel( m_splitterPlotAndConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelConsole->SetMinSize( wxSize( -1,100 ) );
	
	m_sizerConsole = new wxBoxSizer( wxVERTICAL );
	
	m_simConsole = new wxTextCtrl( m_panelConsole, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	m_simConsole->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	m_sizerConsole->Add( m_simConsole, 1, wxALL|wxEXPAND, 5 );
	
	
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
	
	m_splitterSignals = new wxSplitterWindow( m_sidePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterSignals->SetSashGravity( 0.3 );
	m_splitterSignals->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterSignalsOnIdle ), NULL, this );
	m_splitterSignals->SetMinimumPaneSize( 20 );
	
	m_panelSignals = new wxPanel( m_splitterSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelSignals->SetMinSize( wxSize( -1,100 ) );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_panelSignals, wxID_ANY, _("Signals") ), wxVERTICAL );
	
	m_signals = new wxListView( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	sbSizer1->Add( m_signals, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panelSignals->SetSizer( sbSizer1 );
	m_panelSignals->Layout();
	sbSizer1->Fit( m_panelSignals );
	m_panelCursorsAndTune = new wxPanel( m_splitterSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelCursorsAndTune->SetMinSize( wxSize( -1,300 ) );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_splitterTuneValues = new wxSplitterWindow( m_panelCursorsAndTune, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterTuneValues->SetSashGravity( 0.5 );
	m_splitterTuneValues->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterTuneValuesOnIdle ), NULL, this );
	m_splitterTuneValues->SetMinimumPaneSize( 20 );
	
	m_panelCursors = new wxPanel( m_splitterTuneValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelCursors->SetMinSize( wxSize( -1,100 ) );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_panelCursors, wxID_ANY, _("Cursors") ), wxVERTICAL );
	
	m_cursors = new wxListCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL );
	sbSizer3->Add( m_cursors, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panelCursors->SetSizer( sbSizer3 );
	m_panelCursors->Layout();
	sbSizer3->Fit( m_panelCursors );
	m_tunePanel = new wxPanel( m_splitterTuneValues, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_tunePanel->SetMinSize( wxSize( -1,200 ) );
	
	m_tuneSizerStb = new wxStaticBoxSizer( new wxStaticBox( m_tunePanel, wxID_ANY, _("Tune") ), wxHORIZONTAL );
	
	m_tuneSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	m_tuneSizerStb->Add( m_tuneSizer, 1, wxEXPAND, 5 );
	
	
	m_tunePanel->SetSizer( m_tuneSizerStb );
	m_tunePanel->Layout();
	m_tuneSizerStb->Fit( m_tunePanel );
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
	this->Connect( m_newPlot->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuNewPlot ) );
	this->Connect( m_openWorkbook->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuOpenWorkbook ) );
	this->Connect( m_saveWorkbook->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuSaveWorkbook ) );
	this->Connect( m_saveImage->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuSaveImage ) );
	this->Connect( m_saveCsv->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuSaveCsv ) );
	this->Connect( m_exitSim->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuExit ) );
	this->Connect( m_zoomIn->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuZoomIn ) );
	this->Connect( m_zoomOut->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuZoomOut ) );
	this->Connect( m_zoomFit->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuZoomFit ) );
	this->Connect( m_showGrid->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuShowGrid ) );
	this->Connect( m_showGrid->GetId(), wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIM_PLOT_FRAME_BASE::menuShowGridUpdate ) );
	this->Connect( m_showLegend->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuShowLegend ) );
	this->Connect( m_showLegend->GetId(), wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIM_PLOT_FRAME_BASE::menuShowLegendUpdate ) );
	m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotChanged ), NULL, this );
	m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotClose ), NULL, this );
	m_signals->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SIM_PLOT_FRAME_BASE::onSignalDblClick ), NULL, this );
	m_signals->Connect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( SIM_PLOT_FRAME_BASE::onSignalRClick ), NULL, this );
}

SIM_PLOT_FRAME_BASE::~SIM_PLOT_FRAME_BASE()
{
	// Disconnect Events
	this->Disconnect( wxID_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuNewPlot ) );
	this->Disconnect( wxID_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuOpenWorkbook ) );
	this->Disconnect( wxID_SAVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuSaveWorkbook ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuSaveImage ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuSaveCsv ) );
	this->Disconnect( wxID_CLOSE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuExit ) );
	this->Disconnect( wxID_ZOOM_IN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuZoomIn ) );
	this->Disconnect( wxID_ZOOM_OUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuZoomOut ) );
	this->Disconnect( wxID_ZOOM_FIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuZoomFit ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuShowGrid ) );
	this->Disconnect( wxID_ANY, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIM_PLOT_FRAME_BASE::menuShowGridUpdate ) );
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuShowLegend ) );
	this->Disconnect( wxID_ANY, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIM_PLOT_FRAME_BASE::menuShowLegendUpdate ) );
	m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotChanged ), NULL, this );
	m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE, wxAuiNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotClose ), NULL, this );
	m_signals->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( SIM_PLOT_FRAME_BASE::onSignalDblClick ), NULL, this );
	m_signals->Disconnect( wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, wxListEventHandler( SIM_PLOT_FRAME_BASE::onSignalRClick ), NULL, this );
	
}
