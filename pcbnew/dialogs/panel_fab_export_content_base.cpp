///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_fab_export_content_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_FAB_EXPORT_CONTENT_BASE::PANEL_FAB_EXPORT_CONTENT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : WX_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerRightCol;
	bSizerRightCol = new wxBoxSizer( wxVERTICAL );

	m_columnsLabel = new wxStaticText( this, wxID_ANY, _("Content"), wxDefaultPosition, wxDefaultSize, 0 );
	m_columnsLabel->Wrap( -1 );
	bSizerRightCol->Add( m_columnsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 8 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerRightCol->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer4;
	fgSizer4 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer4->AddGrowableCol( 1 );
	fgSizer4->SetFlexibleDirection( wxBOTH );
	fgSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblDataSet = new wxStaticText( this, wxID_ANY, _("Data set:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblDataSet->Wrap( -1 );
	fgSizer4->Add( m_lblDataSet, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceDataSetChoices[] = { _("All data (user-defined)"), _("Bill of materials"), _("Stackup"), _("Fabrication"), _("Assembly"), _("Test"), _("Stencil") };
	int m_choiceDataSetNChoices = sizeof( m_choiceDataSetChoices ) / sizeof( wxString );
	m_choiceDataSet = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceDataSetNChoices, m_choiceDataSetChoices, 0 );
	m_choiceDataSet->SetSelection( 0 );
	m_choiceDataSet->SetToolTip( _("Which sections of the design the file carries, per the IPC-2581 function mode table") );

	fgSizer4->Add( m_choiceDataSet, 0, wxEXPAND|wxRIGHT, 5 );

	m_lblNetNames = new wxStaticText( this, wxID_ANY, _("Net names:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblNetNames->Wrap( -1 );
	fgSizer4->Add( m_lblNetNames, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceNetNamesChoices[] = { _("Include"), _("Anonymize") };
	int m_choiceNetNamesNChoices = sizeof( m_choiceNetNamesChoices ) / sizeof( wxString );
	m_choiceNetNames = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceNetNamesNChoices, m_choiceNetNamesChoices, 0 );
	m_choiceNetNames->SetSelection( 0 );
	m_choiceNetNames->SetToolTip( _("Anonymized names keep connectivity for netlist compare without carrying the design intent") );

	fgSizer4->Add( m_choiceNetNames, 0, wxEXPAND|wxRIGHT, 5 );

	m_lblVariant = new wxStaticText( this, wxID_ANY, _("Variant:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblVariant->Wrap( -1 );
	fgSizer4->Add( m_lblVariant, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceVariantChoices[] = { _("Current") };
	int m_choiceVariantNChoices = sizeof( m_choiceVariantChoices ) / sizeof( wxString );
	m_choiceVariant = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceVariantNChoices, m_choiceVariantChoices, 0 );
	m_choiceVariant->SetSelection( 0 );
	m_choiceVariant->SetToolTip( _("Select a board variant for this export") );

	fgSizer4->Add( m_choiceVariant, 0, wxEXPAND|wxRIGHT, 5 );

	m_lblRefDes = new wxStaticText( this, wxID_ANY, _("Reference designators:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblRefDes->Wrap( -1 );
	fgSizer4->Add( m_lblRefDes, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	wxString m_choiceRefDesChoices[] = { _("Include"), _("Omit") };
	int m_choiceRefDesNChoices = sizeof( m_choiceRefDesChoices ) / sizeof( wxString );
	m_choiceRefDes = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceRefDesNChoices, m_choiceRefDesChoices, 0 );
	m_choiceRefDes->SetSelection( 0 );
	m_choiceRefDes->SetToolTip( _("Omits the designator from the component, BOM and artwork metadata. Reference text drawn on silkscreen is artwork the fabricator must reproduce and is always kept.") );

	fgSizer4->Add( m_choiceRefDes, 0, wxEXPAND|wxRIGHT, 5 );


	bSizerRightCol->Add( fgSizer4, 1, wxALL|wxEXPAND, 5 );

	wxString m_variantOutputChoices[] = { _("One package per variant"), _("One package, all variants inside") };
	int m_variantOutputNChoices = sizeof( m_variantOutputChoices ) / sizeof( wxString );
	m_variantOutput = new wxRadioBox( this, wxID_ANY, _("Variant output"), wxDefaultPosition, wxDefaultSize, m_variantOutputNChoices, m_variantOutputChoices, 1, wxRA_SPECIFY_COLS );
	m_variantOutput->SetSelection( 0 );
	bSizerRightCol->Add( m_variantOutput, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_variantHint = new wxStaticText( this, wxID_ANY, _("${VARIANT} is replaced per package"), wxDefaultPosition, wxDefaultSize, 0 );
	m_variantHint->Wrap( 280 );
	bSizerRightCol->Add( m_variantHint, 0, wxLEFT|wxRIGHT, 5 );

	m_lblIncludes = new wxStaticText( this, wxID_ANY, _("Includes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblIncludes->Wrap( 280 );
	bSizerRightCol->Add( m_lblIncludes, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	bSizerContentButtons = new wxBoxSizer( wxHORIZONTAL );

	m_btnCustomize = new wxButton( this, wxID_ANY, _("Customize..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerContentButtons->Add( m_btnCustomize, 0, wxRIGHT, 5 );


	bSizerContentButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_btnBomFields = new wxButton( this, wxID_ANY, _("BOM Fields..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerContentButtons->Add( m_btnBomFields, 0, wxLEFT, 5 );


	bSizerRightCol->Add( bSizerContentButtons, 0, wxEXPAND|wxTOP|wxRIGHT, 5 );


	this->SetSizer( bSizerRightCol );
	this->Layout();
	bSizerRightCol->Fit( this );

	// Connect Events
	m_choiceDataSet->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onDataSetChange ), NULL, this );
	m_choiceVariant->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onVariantChange ), NULL, this );
	m_variantOutput->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onVariantOutputChange ), NULL, this );
	m_btnCustomize->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onCustomizeClick ), NULL, this );
	m_btnBomFields->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onBomFieldsClick ), NULL, this );
}

PANEL_FAB_EXPORT_CONTENT_BASE::~PANEL_FAB_EXPORT_CONTENT_BASE()
{
	// Disconnect Events
	m_choiceDataSet->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onDataSetChange ), NULL, this );
	m_choiceVariant->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onVariantChange ), NULL, this );
	m_variantOutput->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onVariantOutputChange ), NULL, this );
	m_btnCustomize->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onCustomizeClick ), NULL, this );
	m_btnBomFields->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_FAB_EXPORT_CONTENT_BASE::onBomFieldsClick ), NULL, this );

}

DIALOG_FAB_CUSTOMIZE_BASE::DIALOG_FAB_CUSTOMIZE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_intro = new wxStaticText( this, wxID_ANY, _("Choose the optional sections to include"), wxDefaultPosition, wxDefaultSize, 0 );
	m_intro->Wrap( -1 );
	bMainSizer->Add( m_intro, 0, wxALL, 10 );

	m_sections = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES|wxDV_VERT_RULES|wxBORDER_SIMPLE );
	m_sections->SetMinSize( wxSize( 520,410 ) );

	bMainSizer->Add( m_sections, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_FAB_CUSTOMIZE_BASE::~DIALOG_FAB_CUSTOMIZE_BASE()
{
}
