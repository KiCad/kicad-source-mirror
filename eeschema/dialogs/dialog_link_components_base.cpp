///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_link_components_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LINK_COMPONENTS_BASE::DIALOG_LINK_COMPONENTS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 0, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_filterSource = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_filterSource, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_filterDest = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_filterDest, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_listSources = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_SINGLE|wxLB_SORT );
	gbSizer2->Add( m_listSources, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_listTargets = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_EXTENDED|wxLB_MULTIPLE );
	gbSizer2->Add( m_listTargets, wxGBPosition( 3, 1 ), wxGBSpan( 3, 1 ), wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("Filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	bSizer10->Add( m_staticText21, 0, wxALL, 5 );

	m_filterNets = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer10->Add( m_filterNets, 0, wxALL|wxEXPAND, 5 );


	gbSizer2->Add( bSizer10, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_srcNets = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_ALWAYS_SB|wxLB_EXTENDED|wxLB_MULTIPLE );
	gbSizer2->Add( m_srcNets, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	m_gridKiLinks = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_gridKiLinks->CreateGrid( 5, 5 );
	m_gridKiLinks->EnableEditing( true );
	m_gridKiLinks->EnableGridLines( true );
	m_gridKiLinks->EnableDragGridSize( false );
	m_gridKiLinks->SetMargins( 0, 0 );

	// Columns
	m_gridKiLinks->EnableDragColMove( false );
	m_gridKiLinks->EnableDragColSize( true );
	m_gridKiLinks->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_gridKiLinks->EnableDragRowSize( true );
	m_gridKiLinks->SetRowLabelSize( 0 );
	m_gridKiLinks->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_gridKiLinks->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer11->Add( m_gridKiLinks, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText18 = new wxStaticText( this, wxID_ANY, _("Add created links to LinkClass: "), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText18->Wrap( -1 );
	bSizer12->Add( m_staticText18, 0, wxALL, 5 );


	bSizer12->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbLinkClass = new wxComboBox( this, wxID_ANY, _("dummy"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer12->Add( m_cbLinkClass, 0, wxALL|wxEXPAND, 5 );


	bSizer11->Add( bSizer12, 1, wxEXPAND, 5 );


	gbSizer2->Add( bSizer11, wxGBPosition( 6, 0 ), wxGBSpan( 1, 2 ), wxEXPAND, 5 );

	m_txtSource = new wxStaticText( this, wxID_ANY, _("Source Component"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtSource->Wrap( -1 );
	gbSizer2->Add( m_txtSource, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT|wxTOP, 5 );

	m_txtDest = new wxStaticText( this, wxID_ANY, _("Target Components"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtDest->Wrap( -1 );
	gbSizer2->Add( m_txtDest, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxLEFT|wxRIGHT|wxTOP, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	gbSizer2->Add( m_staticline1, wxGBPosition( 1, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizer14->Add( gbSizer2, 1, wxEXPAND, 5 );

	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();

	bSizer14->Add( m_sdbSizer2, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer14 );
	this->Layout();
	bSizer14->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnSize ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnUpdateUI ) );
	m_filterSource->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnSrcFilter ), NULL, this );
	m_filterDest->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnTargetFilter ), NULL, this );
	m_listSources->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnSrcSelect ), NULL, this );
	m_listTargets->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnTargetSelect ), NULL, this );
	m_filterNets->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnNetFilter ), NULL, this );
	m_srcNets->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnNetSelect ), NULL, this );
	m_gridKiLinks->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnGridCellClick ), NULL, this );
	m_gridKiLinks->Connect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnGridLabelClick ), NULL, this );
	m_sdbSizer2Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnCancel ), NULL, this );
	m_sdbSizer2OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnOK ), NULL, this );
}

DIALOG_LINK_COMPONENTS_BASE::~DIALOG_LINK_COMPONENTS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnSize ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnUpdateUI ) );
	m_filterSource->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnSrcFilter ), NULL, this );
	m_filterDest->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnTargetFilter ), NULL, this );
	m_listSources->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnSrcSelect ), NULL, this );
	m_listTargets->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnTargetSelect ), NULL, this );
	m_filterNets->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnNetFilter ), NULL, this );
	m_srcNets->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnNetSelect ), NULL, this );
	m_gridKiLinks->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnGridCellClick ), NULL, this );
	m_gridKiLinks->Disconnect( wxEVT_GRID_LABEL_LEFT_CLICK, wxGridEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnGridLabelClick ), NULL, this );
	m_sdbSizer2Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnCancel ), NULL, this );
	m_sdbSizer2OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LINK_COMPONENTS_BASE::OnOK ), NULL, this );

}
