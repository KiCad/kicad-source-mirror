///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_grid.h"
#include "widgets/wx_infobar.h"

#include "dialog_user_defined_signals_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_USER_DEFINED_SIGNALS_BASE::DIALOG_USER_DEFINED_SIGNALS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_infoBar = new WX_INFOBAR( this );
	m_infoBar->SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
	m_infoBar->SetEffectDuration( 500 );
	m_infoBar->Hide();

	bMainSizer->Add( m_infoBar, 0, wxEXPAND|wxBOTTOM, 5 );

	m_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid->CreateGrid( 0, 2 );
	m_grid->EnableEditing( true );
	m_grid->EnableGridLines( true );
	m_grid->EnableDragGridSize( false );
	m_grid->SetMargins( 0, 0 );

	// Columns
	m_grid->SetColSize( 0, 400 );
	m_grid->SetColSize( 1, 0 );
	m_grid->EnableDragColMove( false );
	m_grid->EnableDragColSize( true );
	m_grid->SetColLabelSize( 0 );
	m_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid->EnableDragRowSize( false );
	m_grid->SetRowLabelSize( 0 );
	m_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_grid->SetMinSize( wxSize( -1,120 ) );

	bMainSizer->Add( m_grid, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	bButtonSize = new wxBoxSizer( wxHORIZONTAL );

	m_addButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_addButton->SetToolTip( _("Add field") );

	bButtonSize->Add( m_addButton, 0, wxTOP|wxRIGHT, 5 );


	bButtonSize->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_deleteButton->SetToolTip( _("Delete field") );

	bButtonSize->Add( m_deleteButton, 0, wxTOP|wxRIGHT|wxLEFT, 5 );


	bButtonSize->Add( 0, 0, 1, wxEXPAND, 5 );

	m_syntaxHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_syntaxHelp->SetToolTip( _("Show syntax help window") );

	bButtonSize->Add( m_syntaxHelp, 0, wxTOP|wxRIGHT, 4 );


	bMainSizer->Add( bButtonSize, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::onGridCellClick ), NULL, this );
	m_addButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::onAddSignal ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::onDeleteSignal ), NULL, this );
	m_syntaxHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::OnFormattingHelp ), NULL, this );
}

DIALOG_USER_DEFINED_SIGNALS_BASE::~DIALOG_USER_DEFINED_SIGNALS_BASE()
{
	// Disconnect Events
	m_grid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::onGridCellClick ), NULL, this );
	m_addButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::onAddSignal ), NULL, this );
	m_deleteButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::onDeleteSignal ), NULL, this );
	m_syntaxHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_USER_DEFINED_SIGNALS_BASE::OnFormattingHelp ), NULL, this );

}
