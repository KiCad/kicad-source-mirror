///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 31 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "sim_plot_frame_base.h"

///////////////////////////////////////////////////////////////////////////

SIM_PLOT_FRAME_BASE::SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : KIWAY_PLAYER( parent, id, title, pos, size, style, name )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
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
	
	m_sizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_toolBar = new wxToolBar( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxTB_TEXT ); 
	m_toolBar->Realize(); 
	
	m_sizer1->Add( m_toolBar, 0, wxEXPAND, 5 );
	
	m_splitterPlot = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterPlot->SetSashGravity( 0.8 );
	m_splitterPlot->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterPlotOnIdle ), NULL, this );
	
	m_panel2 = new wxPanel( m_splitterPlot, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sizer11 = new wxBoxSizer( wxVERTICAL );
	
	m_splitterConsole = new wxSplitterWindow( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	m_splitterConsole->SetSashGravity( 0.8 );
	m_splitterConsole->Connect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitterConsoleOnIdle ), NULL, this );
	
	m_plotPanel = new wxPanel( m_splitterConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sizer5 = new wxBoxSizer( wxHORIZONTAL );
	
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
	
	m_sizer5->Add( m_plotNotebook, 1, wxEXPAND | wxALL, 5 );
	
	
	m_plotPanel->SetSizer( m_sizer5 );
	m_plotPanel->Layout();
	m_sizer5->Fit( m_plotPanel );
	m_panel5 = new wxPanel( m_splitterConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sizer13 = new wxBoxSizer( wxVERTICAL );
	
	m_simConsole = new wxTextCtrl( m_panel5, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP|wxTE_MULTILINE|wxTE_READONLY );
	m_simConsole->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	m_sizer13->Add( m_simConsole, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel5->SetSizer( m_sizer13 );
	m_panel5->Layout();
	m_sizer13->Fit( m_panel5 );
	m_splitterConsole->SplitHorizontally( m_plotPanel, m_panel5, 500 );
	m_sizer11->Add( m_splitterConsole, 1, wxEXPAND, 5 );
	
	
	m_panel2->SetSizer( m_sizer11 );
	m_panel2->Layout();
	m_sizer11->Fit( m_panel2 );
	m_sidePanel = new wxPanel( m_splitterPlot, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_sideSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_sidePanel, wxID_ANY, _("Signals") ), wxVERTICAL );
	
	m_signals = new wxListView( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL );
	sbSizer1->Add( m_signals, 1, wxALL|wxEXPAND, 5 );
	
	
	m_sideSizer->Add( sbSizer1, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_sidePanel, wxID_ANY, _("Cursors") ), wxVERTICAL );
	
	m_cursors = new wxListCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL );
	sbSizer3->Add( m_cursors, 1, wxALL|wxEXPAND, 5 );
	
	
	m_sideSizer->Add( sbSizer3, 1, wxEXPAND, 5 );
	
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_sidePanel, wxID_ANY, _("Tune") ), wxVERTICAL );
	
	m_tuneSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	sbSizer4->Add( m_tuneSizer, 1, wxEXPAND, 5 );
	
	
	m_sideSizer->Add( sbSizer4, 1, wxEXPAND, 5 );
	
	
	m_sidePanel->SetSizer( m_sideSizer );
	m_sidePanel->Layout();
	m_sideSizer->Fit( m_sidePanel );
	m_splitterPlot->SplitVertically( m_panel2, m_sidePanel, 700 );
	m_sizer1->Add( m_splitterPlot, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( m_sizer1 );
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
