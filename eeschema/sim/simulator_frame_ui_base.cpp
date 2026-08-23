///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "simulator_frame_ui_base.h"

///////////////////////////////////////////////////////////////////////////

SIMULATOR_FRAME_UI_BASE::SIMULATOR_FRAME_UI_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                                  const wxSize& size, long style, const wxString& name ) :
        wxPanel( parent, id, pos, size, style, name )
{
    wxBoxSizer* sizerMain;
    sizerMain = new wxBoxSizer( wxVERTICAL );

    m_splitterLeftRight = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                wxSP_3DSASH | wxSP_LIVE_UPDATE | wxBORDER_NONE );
    m_splitterLeftRight->SetSashGravity( 0.0 );
    // NB: the wxFormBuilder-generated idle handlers that re-apply the designed sash positions
    // are deliberately not connected: they fire on the first idle, which can land *after*
    // SIMULATOR_FRAME_UI::SetSubWindowsSashSize() has restored the user's saved positions, and
    // would silently reset them (m_splitterCursors' design default is 0, which is why the
    // measurements panel never kept its size).  The Split*() calls below set the defaults.
    m_splitterLeftRight->SetMinimumPaneSize( 50 );

    // LEFT column: filterable signal list (grouped by kind) + tune sliders.
    m_sidePanel = new wxPanel( m_splitterLeftRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_sidePanel->SetMinSize( wxSize( 260, -1 ) );

    m_sideSizer = new wxBoxSizer( wxVERTICAL );

    m_splitterSignals = new wxSplitterWindow( m_sidePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxSP_3DSASH | wxSP_LIVE_UPDATE );
    m_splitterSignals->SetSashGravity( 0.7 );
    m_splitterSignals->SetMinimumPaneSize( 20 );

    m_panelSignals = new wxPanel( m_splitterSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelSignals->SetMinSize( wxSize( -1, 100 ) );

    wxBoxSizer* bSizerSignals;
    bSizerSignals = new wxBoxSizer( wxVERTICAL );

    m_filter = new wxSearchCtrl( m_panelSignals, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
#ifndef __WXMAC__
    m_filter->ShowSearchButton( true );
#endif
    m_filter->ShowCancelButton( true );
    bSizerSignals->Add( m_filter, 0, wxEXPAND, 5 );

    // The signals are shown grouped by kind (voltages, currents, ...) in a tree; the columns
    // and per-signal actions the old flat grid carried are reached from the context menu.
    m_signalsTree = new wxDataViewCtrl( m_panelSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxDV_ROW_LINES | wxDV_MULTIPLE | wxBORDER_NONE );
    bSizerSignals->Add( m_signalsTree, 1, wxEXPAND, 5 );


    m_panelSignals->SetSizer( bSizerSignals );
    m_panelSignals->Layout();
    bSizerSignals->Fit( m_panelSignals );
    m_panelTuners = new wxPanel( m_splitterSignals, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelTuners->SetMinSize( wxSize( -1, 100 ) );

    m_sizerTuners = new wxBoxSizer( wxVERTICAL );


    m_panelTuners->SetSizer( m_sizerTuners );
    m_panelTuners->Layout();
    m_sizerTuners->Fit( m_panelTuners );
    m_splitterSignals->SplitHorizontally( m_panelSignals, m_panelTuners, 0 );
    m_sideSizer->Add( m_splitterSignals, 1, wxEXPAND, 5 );


    m_sidePanel->SetSizer( m_sideSizer );
    m_sidePanel->Layout();
    m_sideSizer->Fit( m_sidePanel );
    // CENTER/RIGHT: plot+console on the left, cursors+measurements on the right.
    m_splitterCenterRight = new wxSplitterWindow( m_splitterLeftRight, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                  wxSP_3DSASH | wxSP_LIVE_UPDATE | wxBORDER_NONE );
    m_splitterCenterRight->SetSashGravity( 0.7 );
    m_splitterCenterRight->SetMinimumPaneSize( 50 );

    m_panelLeft = new wxPanel( m_splitterCenterRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelLeft->SetMinSize( wxSize( 300, -1 ) );

    m_sizer11 = new wxBoxSizer( wxVERTICAL );

    m_splitterPlotAndConsole = new wxSplitterWindow( m_panelLeft, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                     wxSP_3DSASH | wxSP_LIVE_UPDATE );
    m_splitterPlotAndConsole->SetSashGravity( 0.8 );
    m_splitterPlotAndConsole->SetMinimumPaneSize( 50 );

    m_plotPanel = new wxPanel( m_splitterPlotAndConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_plotPanel->SetMinSize( wxSize( -1, 200 ) );

    m_sizerPlot = new wxBoxSizer( wxHORIZONTAL );

    m_plotNotebook = new wxAuiNotebook( m_plotPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxAUI_NB_CLOSE_ON_ALL_TABS | wxAUI_NB_MIDDLE_CLICK_CLOSE | wxAUI_NB_TAB_MOVE
                                                | wxAUI_NB_TOP );
    m_plotNotebook->SetMinSize( wxSize( 200, 200 ) );


    m_sizerPlot->Add( m_plotNotebook, 1, wxEXPAND, 5 );


    m_plotPanel->SetSizer( m_sizerPlot );
    m_plotPanel->Layout();
    m_sizerPlot->Fit( m_plotPanel );
    m_panelConsole =
            new wxPanel( m_splitterPlotAndConsole, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelConsole->SetMinSize( wxSize( -1, 100 ) );

    m_sizerConsole = new wxBoxSizer( wxVERTICAL );

    m_sizerConsoleFilters = new wxBoxSizer( wxHORIZONTAL );

    m_consoleFilterAll =
            new wxToggleButton( m_panelConsole, wxID_ANY, _( "All" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_consoleFilterAll->SetValue( true );
    m_sizerConsoleFilters->Add( m_consoleFilterAll, 0, wxALL, 2 );

    m_consoleFilterWarnings =
            new wxToggleButton( m_panelConsole, wxID_ANY, _( "Warnings" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizerConsoleFilters->Add( m_consoleFilterWarnings, 0, wxALL, 2 );

    m_consoleFilterErrors =
            new wxToggleButton( m_panelConsole, wxID_ANY, _( "Errors" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_sizerConsoleFilters->Add( m_consoleFilterErrors, 0, wxALL, 2 );

    m_sizerConsole->Add( m_sizerConsoleFilters, 0, wxEXPAND, 5 );

    m_simConsole = new wxTextCtrl( m_panelConsole, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                   wxTE_DONTWRAP | wxTE_MULTILINE | wxTE_READONLY );
    m_simConsole->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                                   wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
    m_simConsole->SetMinSize( wxSize( -1, 100 ) );

    m_sizerConsole->Add( m_simConsole, 1, wxEXPAND, 5 );


    m_panelConsole->SetSizer( m_sizerConsole );
    m_panelConsole->Layout();
    m_sizerConsole->Fit( m_panelConsole );
    m_splitterPlotAndConsole->SplitHorizontally( m_plotPanel, m_panelConsole, 500 );
    m_sizer11->Add( m_splitterPlotAndConsole, 1, wxEXPAND, 5 );


    m_panelLeft->SetSizer( m_sizer11 );
    m_panelLeft->Layout();
    m_sizer11->Fit( m_panelLeft );
    m_panelCMT = new wxPanel( m_splitterCenterRight, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelCMT->SetMinSize( wxSize( 220, -1 ) );

    wxBoxSizer* bSizerCMT;
    bSizerCMT = new wxBoxSizer( wxVERTICAL );

    m_splitterCursors = new wxSplitterWindow( m_panelCMT, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxSP_3DSASH | wxSP_LIVE_UPDATE | wxBORDER_NONE );
    m_splitterCursors->SetSashGravity( 0.3 );
    m_splitterCursors->SetMinimumPaneSize( 20 );

    m_panelCursors = new wxPanel( m_splitterCursors, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelCursors->SetMinSize( wxSize( -1, 60 ) );

    wxBoxSizer* bSizerCursors;
    bSizerCursors = new wxBoxSizer( wxVERTICAL );

    m_cursorsGrid = new WX_GRID( m_panelCursors, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

    // Grid
    m_cursorsGrid->CreateGrid( 0, 4 );
    m_cursorsGrid->EnableEditing( true );
    m_cursorsGrid->EnableGridLines( true );
    m_cursorsGrid->EnableDragGridSize( false );
    m_cursorsGrid->SetMargins( 0, 0 );

    // Columns
    m_cursorsGrid->SetColSize( 0, 45 );
    m_cursorsGrid->SetColSize( 1, 162 );
    m_cursorsGrid->SetColSize( 2, 90 );
    m_cursorsGrid->SetColSize( 3, 90 );
    m_cursorsGrid->EnableDragColMove( false );
    m_cursorsGrid->EnableDragColSize( true );
    m_cursorsGrid->SetColLabelValue( 0, _( "Cursor" ) );
    m_cursorsGrid->SetColLabelValue( 1, _( "Signal" ) );
    m_cursorsGrid->SetColLabelValue( 2, _( "Time" ) );
    m_cursorsGrid->SetColLabelValue( 3, _( "Value" ) );
    m_cursorsGrid->SetColLabelSize( -1 );
    m_cursorsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Rows
    m_cursorsGrid->EnableDragRowSize( true );
    m_cursorsGrid->SetRowLabelSize( 0 );
    m_cursorsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Label Appearance

    // Cell Defaults
    m_cursorsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
    bSizerCursors->Add( m_cursorsGrid, 1, wxEXPAND, 5 );


    m_panelCursors->SetSizer( bSizerCursors );
    m_panelCursors->Layout();
    bSizerCursors->Fit( m_panelCursors );
    m_panelMeasurements = new wxPanel( m_splitterCursors, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    m_panelMeasurements->SetMinSize( wxSize( -1, 20 ) );

    wxBoxSizer* bSizerMeasurements;
    bSizerMeasurements = new wxBoxSizer( wxVERTICAL );

    // Measurements are grouped by the signal they're taken on; the value column carries the
    // result and the format is reached from the context menu.
    m_measurementsTree = new wxDataViewCtrl( m_panelMeasurements, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             wxDV_ROW_LINES | wxDV_MULTIPLE | wxBORDER_NONE );
    bSizerMeasurements->Add( m_measurementsTree, 1, wxEXPAND, 5 );


    m_panelMeasurements->SetSizer( bSizerMeasurements );
    m_panelMeasurements->Layout();
    bSizerMeasurements->Fit( m_panelMeasurements );
    m_splitterCursors->SplitHorizontally( m_panelCursors, m_panelMeasurements, 0 );
    bSizerCMT->Add( m_splitterCursors, 1, wxEXPAND, 5 );


    m_panelCMT->SetSizer( bSizerCMT );
    m_panelCMT->Layout();
    bSizerCMT->Fit( m_panelCMT );
    m_splitterCenterRight->SplitVertically( m_panelLeft, m_panelCMT, 700 );

    m_splitterLeftRight->SplitVertically( m_sidePanel, m_splitterCenterRight, 300 );
    sizerMain->Add( m_splitterLeftRight, 1, wxEXPAND, 5 );


    this->SetSizer( sizerMain );
    this->Layout();
    sizerMain->Fit( this );

    // Connect Events
    this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIMULATOR_FRAME_UI_BASE::OnUpdateUI ) );
    m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_END_DRAG,
                             wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotDragged ), NULL, this );
    m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
                             wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotChanged ), NULL, this );
    m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING,
                             wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotChanging ), NULL, this );
    m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE,
                             wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotClose ), NULL, this );
    m_plotNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED,
                             wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotClosed ), NULL, this );
    m_filter->Connect( wxEVT_MOTION, wxMouseEventHandler( SIMULATOR_FRAME_UI_BASE::OnFilterMouseMoved ), NULL, this );
    m_filter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnFilterText ), NULL,
                       this );
    m_signalsTree->Connect( wxEVT_DATAVIEW_ITEM_ACTIVATED,
                            wxDataViewEventHandler( SIMULATOR_FRAME_UI_BASE::onSignalsTreeActivated ), NULL, this );
    m_signalsTree->Connect( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                            wxDataViewEventHandler( SIMULATOR_FRAME_UI_BASE::onSignalsTreeContextMenu ), NULL, this );
    m_consoleFilterAll->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
                                 wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnConsoleFilterChanged ), NULL, this );
    m_consoleFilterWarnings->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
                                      wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnConsoleFilterChanged ), NULL,
                                      this );
    m_consoleFilterErrors->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
                                    wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnConsoleFilterChanged ), NULL,
                                    this );
    m_cursorsGrid->Connect( wxEVT_GRID_CELL_CHANGED,
                            wxGridEventHandler( SIMULATOR_FRAME_UI_BASE::onCursorsGridCellChanged ), NULL, this );
    m_measurementsTree->Connect( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                                 wxDataViewEventHandler( SIMULATOR_FRAME_UI_BASE::onMeasurementsTreeContextMenu ), NULL,
                                 this );
}

SIMULATOR_FRAME_UI_BASE::~SIMULATOR_FRAME_UI_BASE()
{
    // Disconnect Events
    this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( SIMULATOR_FRAME_UI_BASE::OnUpdateUI ) );
    m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_END_DRAG,
                                wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotDragged ), NULL, this );
    m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
                                wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotChanged ), NULL, this );
    m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGING,
                                wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotChanging ), NULL, this );
    m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSE,
                                wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotClose ), NULL, this );
    m_plotNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CLOSED,
                                wxAuiNotebookEventHandler( SIMULATOR_FRAME_UI_BASE::onPlotClosed ), NULL, this );
    m_filter->Disconnect( wxEVT_MOTION, wxMouseEventHandler( SIMULATOR_FRAME_UI_BASE::OnFilterMouseMoved ), NULL,
                          this );
    m_filter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnFilterText ),
                          NULL, this );
    m_signalsTree->Disconnect( wxEVT_DATAVIEW_ITEM_ACTIVATED,
                               wxDataViewEventHandler( SIMULATOR_FRAME_UI_BASE::onSignalsTreeActivated ), NULL, this );
    m_signalsTree->Disconnect( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                               wxDataViewEventHandler( SIMULATOR_FRAME_UI_BASE::onSignalsTreeContextMenu ), NULL,
                               this );
    m_consoleFilterAll->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
                                    wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnConsoleFilterChanged ), NULL,
                                    this );
    m_consoleFilterWarnings->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
                                         wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnConsoleFilterChanged ), NULL,
                                         this );
    m_consoleFilterErrors->Disconnect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED,
                                       wxCommandEventHandler( SIMULATOR_FRAME_UI_BASE::OnConsoleFilterChanged ), NULL,
                                       this );
    m_cursorsGrid->Disconnect( wxEVT_GRID_CELL_CHANGED,
                               wxGridEventHandler( SIMULATOR_FRAME_UI_BASE::onCursorsGridCellChanged ), NULL, this );
    m_measurementsTree->Disconnect( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU,
                                    wxDataViewEventHandler( SIMULATOR_FRAME_UI_BASE::onMeasurementsTreeContextMenu ),
                                    NULL, this );
}
