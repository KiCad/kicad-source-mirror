///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_multichannel_repeat_layout_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE::DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 800,-1 ), wxDefaultSize );

	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_GeneralBoxSizer->SetMinSize( wxSize( 800,300 ) );
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->AddGrowableRow( 2 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	fgSizer3->SetMinSize( wxSize( 800,300 ) );
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Target areas:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer3->Add( m_staticText4, 0, wxALL, 5 );

	m_raGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_raGrid->CreateGrid( 1, 3 );
	m_raGrid->EnableEditing( false );
	m_raGrid->EnableGridLines( true );
	m_raGrid->EnableDragGridSize( false );
	m_raGrid->SetMargins( 0, 0 );

	// Columns
	m_raGrid->AutoSizeColumns();
	m_raGrid->EnableDragColMove( true );
	m_raGrid->EnableDragColSize( true );
	m_raGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_raGrid->AutoSizeRows();
	m_raGrid->EnableDragRowSize( true );
	m_raGrid->SetRowLabelSize( wxGRID_AUTOSIZE );
	m_raGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_raGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	fgSizer3->Add( m_raGrid, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Reference Rule Area:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer4->Add( m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_refRAName = new wxStaticText( this, wxID_ANY, _("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_refRAName->Wrap( -1 );
	m_refRAName->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer4->Add( m_refRAName, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3->Add( bSizer4, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );

	m_cbCopyPlacement = new wxCheckBox( this, wxID_ANY, _("Copy footprint placement"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbCopyPlacement, 0, wxALL, 5 );

	m_cbCopyRouting = new wxCheckBox( this, wxID_ANY, _("Copy routing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbCopyRouting, 0, wxALL, 5 );

	m_cbCopyOtherItems = new wxCheckBox( this, wxID_ANY, _("Copy other items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopyOtherItems->SetToolTip( _("Copy text, shapes, zones, and other items inside the source rule area") );

	bSizer13->Add( m_cbCopyOtherItems, 0, wxALL, 5 );

	m_cbGroupItems = new wxCheckBox( this, wxID_ANY, _("Group components with their placement rule areas"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbGroupItems, 0, wxALL, 5 );

	m_cbIncludeLockedComponents = new wxCheckBox( this, wxID_ANY, _("Include locked components"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbIncludeLockedComponents, 0, wxALL, 5 );

	m_cbIncludeOffRAComponents = new wxCheckBox( this, wxID_ANY, _("Include components outside the target area"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_cbIncludeOffRAComponents, 0, wxALL, 5 );


	fgSizer3->Add( bSizer13, 1, wxEXPAND, 5 );

	wxBoxSizer* bottomButtonsSizer;
	bottomButtonsSizer = new wxBoxSizer( wxHORIZONTAL );


	bottomButtonsSizer->Add( 10, 0, 0, 0, 5 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bottomButtonsSizer->Add( m_sdbSizerStdButtons, 1, wxEXPAND|wxALL, 5 );


	fgSizer3->Add( bottomButtonsSizer, 0, wxEXPAND|wxLEFT, 5 );


	m_GeneralBoxSizer->Add( fgSizer3, 1, wxEXPAND, 5 );


	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	m_GeneralBoxSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE::OnInitDlg ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE::OnUpdateUI ) );
}

DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE::~DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE::OnInitDlg ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE::OnUpdateUI ) );

}
