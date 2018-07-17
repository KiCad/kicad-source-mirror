///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_edit_libentry_fields_in_lib_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerFieldsSetup;
	bSizerFieldsSetup = new wxBoxSizer( wxVERTICAL );
	
	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxRAISED_BORDER );
	
	// Grid
	m_grid->CreateGrid( 4, 11 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );
	
	// Columns
	m_grid->SetColSize( 0, 72 );
	m_grid->SetColSize( 1, 120 );
	m_grid->SetColSize( 2, 48 );
	m_grid->SetColSize( 3, 72 );
	m_grid->SetColSize( 4, 72 );
	m_grid->SetColSize( 5, 48 );
	m_grid->SetColSize( 6, 48 );
	m_grid->SetColSize( 7, 84 );
	m_grid->SetColSize( 8, 84 );
	m_grid->SetColSize( 9, 84 );
	m_grid->SetColSize( 10, 84 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 22 );
	m_grid->SetColLabelValue( 0, _("Name") );
	m_grid->SetColLabelValue( 1, _("Value") );
	m_grid->SetColLabelValue( 2, _("Show") );
	m_grid->SetColLabelValue( 3, _("H Align") );
	m_grid->SetColLabelValue( 4, _("V Align") );
	m_grid->SetColLabelValue( 5, _("Italic") );
	m_grid->SetColLabelValue( 6, _("Bold") );
	m_grid->SetColLabelValue( 7, _("Text Size") );
	m_grid->SetColLabelValue( 8, _("Orientation") );
	m_grid->SetColLabelValue( 9, _("X Position") );
	m_grid->SetColLabelValue( 10, _("Y Position") );
	m_grid->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid->EnableDragRowSize( true );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_grid->SetMinSize( wxSize( -1,180 ) );
	
	bSizerFieldsSetup->Add( m_grid, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bButtonSize;
	bButtonSize = new wxBoxSizer( wxHORIZONTAL );
	
	m_bpAdd = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bpAdd->SetMinSize( wxSize( 29,29 ) );
	
	bButtonSize->Add( m_bpAdd, 0, wxRIGHT, 5 );
	
	m_bpDelete = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bpDelete->SetMinSize( wxSize( 29,29 ) );
	
	bButtonSize->Add( m_bpDelete, 0, wxRIGHT, 10 );
	
	m_bpMoveUp = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bpMoveUp->SetMinSize( wxSize( 29,29 ) );
	
	bButtonSize->Add( m_bpMoveUp, 0, wxLEFT, 10 );
	
	m_bpMoveDown = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_bpMoveDown->SetMinSize( wxSize( 29,29 ) );
	
	bButtonSize->Add( m_bpMoveDown, 0, wxRIGHT|wxLEFT, 5 );
	
	
	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerFieldsSetup->Add( bButtonSize, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	mainSizer->Add( bSizerFieldsSetup, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerButtons;
	bSizerButtons = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizerButtons->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_spiceFieldsButton = new wxButton( this, wxID_ANY, _("   Edit Spice Model...   "), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerButtons->Add( m_spiceFieldsButton, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	bSizerButtons->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	stdDialogButtonSizer = new wxStdDialogButtonSizer();
	stdDialogButtonSizerOK = new wxButton( this, wxID_OK );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerOK );
	stdDialogButtonSizerCancel = new wxButton( this, wxID_CANCEL );
	stdDialogButtonSizer->AddButton( stdDialogButtonSizerCancel );
	stdDialogButtonSizer->Realize();
	
	bSizerButtons->Add( stdDialogButtonSizer, 0, wxEXPAND|wxALL, 6 );
	
	
	mainSizer->Add( bSizerButtons, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCloseDialog ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnInitDialog ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnUpdateUI ) );
	m_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnAddField ), NULL, this );
	m_bpDelete->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnDeleteField ), NULL, this );
	m_bpMoveUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnMoveDown ), NULL, this );
	m_spiceFieldsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnEditSpiceModel ), NULL, this );
	stdDialogButtonSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnOKButtonClick ), NULL, this );
}

DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCloseDialog ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnInitDialog ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnUpdateUI ) );
	m_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnSizeGrid ), NULL, this );
	m_bpAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnAddField ), NULL, this );
	m_bpDelete->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnDeleteField ), NULL, this );
	m_bpMoveUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnMoveUp ), NULL, this );
	m_bpMoveDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnMoveDown ), NULL, this );
	m_spiceFieldsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnEditSpiceModel ), NULL, this );
	stdDialogButtonSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnCancelButtonClick ), NULL, this );
	stdDialogButtonSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE::OnOKButtonClick ), NULL, this );
	
}
