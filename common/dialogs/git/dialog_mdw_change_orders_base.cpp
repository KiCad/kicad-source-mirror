///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_mdw_change_orders_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MDW_CHANGE_ORDERS_BASE::DIALOG_MDW_CHANGE_ORDERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 640,400 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_ecoPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_ecoPanelSizer;
	m_ecoPanelSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bEcoMsgSizer;
	bEcoMsgSizer = new wxBoxSizer( wxHORIZONTAL );

	m_ecoMessage = new wxStaticText( m_ecoPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ecoMessage->Wrap( -1 );
	bEcoMsgSizer->Add( m_ecoMessage, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ecoRetry = new wxHyperlinkCtrl( m_ecoPanel, wxID_ANY, _("Retry"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bEcoMsgSizer->Add( m_ecoRetry, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_ecoPanelSizer->Add( bEcoMsgSizer, 0, wxEXPAND, 5 );

	m_showClosed = new wxCheckBox( m_ecoPanel, wxID_ANY, _("Show closed change orders"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ecoPanelSizer->Add( m_showClosed, 0, wxALL, 5 );

	m_ecoGrid = new WX_GRID( m_ecoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_ecoGrid->CreateGrid( 0, 5 );
	m_ecoGrid->EnableEditing( false );
	m_ecoGrid->EnableGridLines( true );
	m_ecoGrid->EnableDragGridSize( false );
	m_ecoGrid->SetMargins( 0, 0 );

	// Columns
	m_ecoGrid->EnableDragColMove( false );
	m_ecoGrid->EnableDragColSize( true );
	m_ecoGrid->SetColLabelValue( 0, _("ID") );
	m_ecoGrid->SetColLabelValue( 1, _("Title") );
	m_ecoGrid->SetColLabelValue( 2, _("State") );
	m_ecoGrid->SetColLabelValue( 3, _("Type") );
	m_ecoGrid->SetColLabelValue( 4, _("Author") );
	m_ecoGrid->SetColLabelSize( 24 );
	m_ecoGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_ecoGrid->EnableDragRowSize( false );
	m_ecoGrid->SetRowLabelSize( 0 );
	m_ecoGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_ecoGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_ecoGrid->SetMinSize( wxSize( 500,200 ) );

	m_ecoPanelSizer->Add( m_ecoGrid, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bEcoButtons;
	bEcoButtons = new wxBoxSizer( wxHORIZONTAL );

	m_ecoDetails = new wxButton( m_ecoPanel, wxID_ANY, _("Details..."), wxDefaultPosition, wxDefaultSize, 0 );
	bEcoButtons->Add( m_ecoDetails, 0, wxALL, 0 );


	m_ecoPanelSizer->Add( bEcoButtons, 0, wxALIGN_RIGHT|wxALL, 5 );


	m_ecoPanel->SetSizer( m_ecoPanelSizer );
	m_ecoPanel->Layout();
	m_ecoPanelSizer->Fit( m_ecoPanel );
	m_notebook->AddPage( m_ecoPanel, _("Change Orders"), true );
	m_reviewPanel = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* m_reviewPanelSizer;
	m_reviewPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_reviewStatus = new wxStaticText( m_reviewPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_reviewStatus->Wrap( -1 );
	m_reviewPanelSizer->Add( m_reviewStatus, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bReviewMsgSizer;
	bReviewMsgSizer = new wxBoxSizer( wxHORIZONTAL );

	m_reviewMessage = new wxStaticText( m_reviewPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_reviewMessage->Wrap( -1 );
	bReviewMsgSizer->Add( m_reviewMessage, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_reviewRetry = new wxHyperlinkCtrl( m_reviewPanel, wxID_ANY, _("Retry"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	bReviewMsgSizer->Add( m_reviewRetry, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_reviewPanelSizer->Add( bReviewMsgSizer, 0, wxEXPAND, 5 );

	m_thisRevisionOnly = new wxCheckBox( m_reviewPanel, wxID_ANY, _("Only this revision"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thisRevisionOnly->SetValue(true);
	m_reviewPanelSizer->Add( m_thisRevisionOnly, 0, wxALL, 5 );

	m_reviewGrid = new WX_GRID( m_reviewPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_reviewGrid->CreateGrid( 0, 5 );
	m_reviewGrid->EnableEditing( false );
	m_reviewGrid->EnableGridLines( true );
	m_reviewGrid->EnableDragGridSize( false );
	m_reviewGrid->SetMargins( 0, 0 );

	// Columns
	m_reviewGrid->EnableDragColMove( false );
	m_reviewGrid->EnableDragColSize( true );
	m_reviewGrid->SetColLabelValue( 0, _("ID") );
	m_reviewGrid->SetColLabelValue( 1, _("Title") );
	m_reviewGrid->SetColLabelValue( 2, _("State") );
	m_reviewGrid->SetColLabelValue( 3, _("Revision") );
	m_reviewGrid->SetColLabelValue( 4, _("Reviewers") );
	m_reviewGrid->SetColLabelSize( 24 );
	m_reviewGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_reviewGrid->EnableDragRowSize( false );
	m_reviewGrid->SetRowLabelSize( 0 );
	m_reviewGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_reviewGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	m_reviewGrid->SetMinSize( wxSize( 500,200 ) );

	m_reviewPanelSizer->Add( m_reviewGrid, 1, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bReviewButtons;
	bReviewButtons = new wxBoxSizer( wxHORIZONTAL );

	m_reviewDetails = new wxButton( m_reviewPanel, wxID_ANY, _("Details..."), wxDefaultPosition, wxDefaultSize, 0 );
	bReviewButtons->Add( m_reviewDetails, 0, wxALL, 0 );


	m_reviewPanelSizer->Add( bReviewButtons, 0, wxALIGN_RIGHT|wxALL, 5 );


	m_reviewPanel->SetSizer( m_reviewPanelSizer );
	m_reviewPanel->Layout();
	m_reviewPanelSizer->Fit( m_reviewPanel );
	m_notebook->AddPage( m_reviewPanel, _("Reviews"), false );

	bMainSizer->Add( m_notebook, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_ecoRetry->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnEcoRetry ), NULL, this );
	m_showClosed->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnShowClosed ), NULL, this );
	m_ecoGrid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnEcoActivated ), NULL, this );
	m_ecoDetails->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnEcoDetails ), NULL, this );
	m_reviewRetry->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnReviewRetry ), NULL, this );
	m_thisRevisionOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnThisRevisionOnly ), NULL, this );
	m_reviewGrid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnReviewActivated ), NULL, this );
	m_reviewDetails->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnReviewDetails ), NULL, this );
}

DIALOG_MDW_CHANGE_ORDERS_BASE::~DIALOG_MDW_CHANGE_ORDERS_BASE()
{
	// Disconnect Events
	m_ecoRetry->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnEcoRetry ), NULL, this );
	m_showClosed->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnShowClosed ), NULL, this );
	m_ecoGrid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnEcoActivated ), NULL, this );
	m_ecoDetails->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnEcoDetails ), NULL, this );
	m_reviewRetry->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnReviewRetry ), NULL, this );
	m_thisRevisionOnly->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnThisRevisionOnly ), NULL, this );
	m_reviewGrid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnReviewActivated ), NULL, this );
	m_reviewDetails->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_MDW_CHANGE_ORDERS_BASE::OnReviewDetails ), NULL, this );

}
