///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 24 2016)
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
	
	wxMenuItem* m_showCoords;
	m_showCoords = new wxMenuItem( m_viewMenu, wxID_ANY, wxString( _("Show &coordinates") ) , wxEmptyString, wxITEM_CHECK );
	m_viewMenu->Append( m_showCoords );
	
	m_mainMenu->Append( m_viewMenu, _("View") ); 
	
	this->SetMenuBar( m_mainMenu );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_plotNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_plotNotebook->SetMinSize( wxSize( 400,-1 ) );
	
	
	bSizer5->Add( m_plotNotebook, 6, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Signals"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
	m_staticText2->Wrap( -1 );
	bSizer7->Add( m_staticText2, 0, wxALL|wxEXPAND, 5 );
	
	m_signals = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_SORT ); 
	bSizer7->Add( m_signals, 1, wxALL|wxEXPAND, 5 );
	
	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Cursors"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	bSizer7->Add( m_staticText21, 0, wxALL, 5 );
	
	m_cursors = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizer7->Add( m_cursors, 1, wxALL|wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 1, 0, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_simulateBtn = new wxButton( this, wxID_ANY, _("Simulate"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_simulateBtn, 0, wxALL, 5 );
	
	m_probeBtn = new wxButton( this, wxID_ANY, _("Probe"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_probeBtn, 0, wxALL|wxEXPAND, 5 );
	
	m_tuneBtn = new wxButton( this, wxID_ANY, _("Tune"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_tuneBtn, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer7->Add( fgSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer5->Add( bSizer7, 1, wxEXPAND, 5 );
	
	
	bSizer1->Add( bSizer5, 3, wxEXPAND, 5 );
	
	m_simConsole = new wxRichTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxVSCROLL|wxHSCROLL|wxNO_BORDER|wxWANTS_CHARS );
	m_simConsole->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	
	bSizer1->Add( m_simConsole, 1, wxEXPAND | wxALL, 5 );
	
	
	this->SetSizer( bSizer1 );
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
	this->Connect( m_showCoords->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuShowCoords ) );
	this->Connect( m_showCoords->GetId(), wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIM_PLOT_FRAME_BASE::menuShowCoordsUpdate ) );
	m_plotNotebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotChanged ), NULL, this );
	m_signals->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onSignalDblClick ), NULL, this );
	m_signals->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( SIM_PLOT_FRAME_BASE::onSignalRClick ), NULL, this );
	m_simulateBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onSimulate ), NULL, this );
	m_probeBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onPlaceProbe ), NULL, this );
	m_tuneBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onTune ), NULL, this );
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
	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::menuShowCoords ) );
	this->Disconnect( wxID_ANY, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIM_PLOT_FRAME_BASE::menuShowCoordsUpdate ) );
	m_plotNotebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( SIM_PLOT_FRAME_BASE::onPlotChanged ), NULL, this );
	m_signals->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onSignalDblClick ), NULL, this );
	m_signals->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( SIM_PLOT_FRAME_BASE::onSignalRClick ), NULL, this );
	m_simulateBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onSimulate ), NULL, this );
	m_probeBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onPlaceProbe ), NULL, this );
	m_tuneBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME_BASE::onTune ), NULL, this );
	
}
