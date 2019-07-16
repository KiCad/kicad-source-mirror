///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 25 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_board_statistics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_STATISTICS_BASE::DIALOG_BOARD_STATISTICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );
    this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );

    wxBoxSizer* bMainBoxSizer;
    bMainBoxSizer = new wxBoxSizer( wxVERTICAL );

    wxGridBagSizer* gbConentsSizer;
    gbConentsSizer = new wxGridBagSizer( 5, 10 );
    gbConentsSizer->SetFlexibleDirection( wxBOTH );
    gbConentsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    wxStaticBoxSizer* sbComponentsSizer;
    sbComponentsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Components") ), wxVERTICAL );

    m_gridComponents = new wxGrid( sbComponentsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

    // Grid
    m_gridComponents->CreateGrid( 5, 4 );
    m_gridComponents->EnableEditing( false );
    m_gridComponents->EnableGridLines( false );
    m_gridComponents->SetGridLineColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );
    m_gridComponents->EnableDragGridSize( false );
    m_gridComponents->SetMargins( 0, 0 );

    // Columns
    m_gridComponents->EnableDragColMove( false );
    m_gridComponents->EnableDragColSize( true );
    m_gridComponents->SetColLabelSize( 0 );
    m_gridComponents->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Rows
    m_gridComponents->EnableDragRowSize( true );
    m_gridComponents->SetRowLabelSize( 0 );
    m_gridComponents->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Label Appearance
    m_gridComponents->SetLabelBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND ) );
    m_gridComponents->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

    // Cell Defaults
    m_gridComponents->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );
    m_gridComponents->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    m_gridComponents->SetMaxSize( wxSize( -1,400 ) );

    sbComponentsSizer->Add( m_gridComponents, 0, wxALIGN_CENTER|wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 5 );


    gbConentsSizer->Add( sbComponentsSizer, wxGBPosition( 0, 0 ), wxGBSpan( 2, 1 ), wxEXPAND, 5 );

    wxStaticBoxSizer* sbPadsSizer;
    sbPadsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pads") ), wxVERTICAL );

    m_gridPads = new wxGrid( sbPadsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

    // Grid
    m_gridPads->CreateGrid( 5, 2 );
    m_gridPads->EnableEditing( false );
    m_gridPads->EnableGridLines( false );
    m_gridPads->SetGridLineColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );
    m_gridPads->EnableDragGridSize( false );
    m_gridPads->SetMargins( 10, 0 );

    // Columns
    m_gridPads->EnableDragColMove( false );
    m_gridPads->EnableDragColSize( true );
    m_gridPads->SetColLabelSize( 0 );
    m_gridPads->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Rows
    m_gridPads->EnableDragRowSize( true );
    m_gridPads->SetRowLabelSize( 0 );
    m_gridPads->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Label Appearance
    m_gridPads->SetLabelBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND ) );
    m_gridPads->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

    // Cell Defaults
    m_gridPads->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );
    m_gridPads->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    m_gridPads->SetMaxSize( wxSize( -1,300 ) );

    sbPadsSizer->Add( m_gridPads, 0, wxALL, 5 );


    gbConentsSizer->Add( sbPadsSizer, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

    wxStaticBoxSizer* sbBoardSizer;
    sbBoardSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Board") ), wxVERTICAL );

    m_gridBoard = new wxGrid( sbBoardSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );

    // Grid
    m_gridBoard->CreateGrid( 3, 2 );
    m_gridBoard->EnableEditing( false );
    m_gridBoard->EnableGridLines( false );
    m_gridBoard->SetGridLineColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );
    m_gridBoard->EnableDragGridSize( false );
    m_gridBoard->SetMargins( 10, 0 );

    // Columns
    m_gridBoard->EnableDragColMove( false );
    m_gridBoard->EnableDragColSize( true );
    m_gridBoard->SetColLabelSize( 0 );
    m_gridBoard->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Rows
    m_gridBoard->EnableDragRowSize( true );
    m_gridBoard->SetRowLabelSize( 0 );
    m_gridBoard->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    // Label Appearance
    m_gridBoard->SetLabelBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND ) );
    m_gridBoard->SetLabelFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

    // Cell Defaults
    m_gridBoard->SetDefaultCellBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWFRAME ) );
    m_gridBoard->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    m_gridBoard->SetMaxSize( wxSize( -1,300 ) );

    sbBoardSizer->Add( m_gridBoard, 0, wxALL, 5 );


    gbConentsSizer->Add( sbBoardSizer, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );


    gbConentsSizer->AddGrowableCol( 0 );
    gbConentsSizer->AddGrowableCol( 1 );
    gbConentsSizer->AddGrowableRow( 0 );

    bMainBoxSizer->Add( gbConentsSizer, 1, wxALIGN_CENTER_HORIZONTAL|wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

    wxGridSizer* gOptionsSizer;
    gOptionsSizer = new wxGridSizer( 0, 2, 0, 0 );

    m_checkBoxSubtractHoles = new wxCheckBox( this, wxID_ANY, _("Subtract holes from board area"), wxDefaultPosition, wxDefaultSize, 0 );
    gOptionsSizer->Add( m_checkBoxSubtractHoles, 0, wxALL|wxEXPAND, 5 );

    m_checkBoxExcludeComponentsNoPins = new wxCheckBox( this, wxID_ANY, _("Exclude components with no pins"), wxDefaultPosition, wxDefaultSize, 0 );
    gOptionsSizer->Add( m_checkBoxExcludeComponentsNoPins, 0, wxALL|wxEXPAND, 5 );


    bMainBoxSizer->Add( gOptionsSizer, 0, wxEXPAND|wxLEFT|wxRIGHT, 10 );

    m_sdbControlSizer = new wxStdDialogButtonSizer();
    m_sdbControlSizerOK = new wxButton( this, wxID_OK );
    m_sdbControlSizer->AddButton( m_sdbControlSizerOK );
    m_sdbControlSizer->Realize();

    bMainBoxSizer->Add( m_sdbControlSizer, 0, wxALIGN_RIGHT|wxBOTTOM|wxLEFT|wxRIGHT|wxTOP, 10 );


    this->SetSizer( bMainBoxSizer );
    this->Layout();
    bMainBoxSizer->Fit( this );

    this->Centre( wxBOTH );

    // Connect Events
    m_checkBoxSubtractHoles->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
    m_checkBoxExcludeComponentsNoPins->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
}

DIALOG_BOARD_STATISTICS_BASE::~DIALOG_BOARD_STATISTICS_BASE()
{
    // Disconnect Events
    m_checkBoxSubtractHoles->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );
    m_checkBoxExcludeComponentsNoPins->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_BOARD_STATISTICS_BASE::checkboxClicked ), NULL, this );

}
