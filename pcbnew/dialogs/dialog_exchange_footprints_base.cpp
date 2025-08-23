///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"
#include "widgets/wx_html_report_panel.h"

#include "dialog_exchange_footprints_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXCHANGE_FOOTPRINTS_BASE::DIALOG_EXCHANGE_FOOTPRINTS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	m_mainSizer = new wxBoxSizer( wxVERTICAL );

	m_upperSizer = new wxGridBagSizer( 0, 0 );
	m_upperSizer->SetFlexibleDirection( wxBOTH );
	m_upperSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_matchAll = new wxRadioButton( this, wxID_ANY, _("Update all footprints on board"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchAll, wxGBPosition( 0, 0 ), wxGBSpan( 1, 2 ), wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_matchSelected = new wxRadioButton( this, wxID_ANY, _("Update selected footprint(s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSelected, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_matchSpecifiedRef = new wxRadioButton( this, wxID_ANY, _("Update footprints matching reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSpecifiedRef, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedRef = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_specifiedRef->SetMinSize( wxSize( 200,-1 ) );

	m_upperSizer->Add( m_specifiedRef, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_matchSpecifiedValue = new wxRadioButton( this, wxID_ANY, _("Update footprints matching value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSpecifiedValue, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_upperSizer->Add( m_specifiedValue, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 2 );

	m_matchSpecifiedID = new wxRadioButton( this, wxID_ANY, _("Update footprints with library id:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_upperSizer->Add( m_matchSpecifiedID, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_specifiedID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizer7->Add( m_specifiedID, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_specifiedIDBrowseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer7->Add( m_specifiedIDBrowseButton, 0, wxALIGN_CENTER_VERTICAL, 2 );


	m_upperSizer->Add( bSizer7, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	m_upperSizer->AddGrowableCol( 1 );

	m_mainSizer->Add( m_upperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );


	m_mainSizer->Add( bSizer4, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_changeSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticLine* staticline1;
	staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_changeSizer->Add( staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 3 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* newIdLabel;
	newIdLabel = new wxStaticText( this, wxID_ANY, _("New footprint library id:"), wxDefaultPosition, wxDefaultSize, 0 );
	newIdLabel->Wrap( -1 );
	bSizer3->Add( newIdLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_newID = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_newID->SetMinSize( wxSize( 500,-1 ) );

	bSizer3->Add( m_newID, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_newIDBrowseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_newIDBrowseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_changeSizer->Add( bSizer3, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	m_changeSizer->Add( 0, 3, 0, wxEXPAND, 5 );


	m_mainSizer->Add( m_changeSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_updateOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Update Options") ), wxHORIZONTAL );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_removeExtraBox = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Remove text items if not in library footprint"), wxDefaultPosition, wxDefaultSize, 0 );
	m_removeExtraBox->SetToolTip( _("Removes fields that do not occur in the original library symbols") );

	bSizer5->Add( m_removeExtraBox, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_resetTextItemLayers = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset text layers and visibilities"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_resetTextItemLayers, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetTextItemEffects = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset text sizes and styles"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_resetTextItemEffects, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetTextItemPositions = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset text positions"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_resetTextItemPositions, 0, wxBOTTOM|wxRIGHT, 5 );

	m_resetTextItemContent = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset text content"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_resetTextItemContent, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer5->Add( 0, 10, 1, 0, 5 );

	m_checkAll = new wxButton( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Check All Update Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_checkAll, 0, wxEXPAND, 5 );


	m_updateOptionsSizer->Add( bSizer5, 1, wxEXPAND|wxRIGHT, 10 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_resetFabricationAttrs = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset fabrication attributes"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_resetFabricationAttrs, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_resetClearanceOverrides = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset clearance overrides"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_resetClearanceOverrides, 0, wxBOTTOM|wxRIGHT, 5 );

	m_reset3DModels = new wxCheckBox( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Update/reset 3D models"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_reset3DModels, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizer6->Add( 0, 10, 1, 0, 5 );

	m_uncheckAll = new wxButton( m_updateOptionsSizer->GetStaticBox(), wxID_ANY, _("Uncheck All Update Options"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( m_uncheckAll, 0, wxEXPAND, 5 );


	m_updateOptionsSizer->Add( bSizer6, 1, wxEXPAND, 5 );


	m_mainSizer->Add( m_updateOptionsSizer, 0, wxEXPAND|wxALL, 10 );

	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,208 ) );

	m_mainSizer->Add( m_MessageWindow, 5, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_mainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_mainSizer );
	this->Layout();
	m_mainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::updateMatchModeRadioButtons ) );
	m_matchAll->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchAllClicked ), NULL, this );
	m_matchSelected->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchSelectedClicked ), NULL, this );
	m_matchSpecifiedRef->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_matchSpecifiedValue->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedID->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedIDBrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_newIDBrowseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_checkAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::onCheckAll ), NULL, this );
	m_uncheckAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::onUncheckAll ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnOKClicked ), NULL, this );
}

DIALOG_EXCHANGE_FOOTPRINTS_BASE::~DIALOG_EXCHANGE_FOOTPRINTS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::updateMatchModeRadioButtons ) );
	m_matchAll->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchAllClicked ), NULL, this );
	m_matchSelected->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchSelectedClicked ), NULL, this );
	m_matchSpecifiedRef->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_specifiedRef->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchRefClicked ), NULL, this );
	m_matchSpecifiedValue->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_specifiedValue->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchValueClicked ), NULL, this );
	m_matchSpecifiedID->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedID->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnMatchIDClicked ), NULL, this );
	m_specifiedIDBrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_newIDBrowseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::ViewAndSelectFootprint ), NULL, this );
	m_checkAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::onCheckAll ), NULL, this );
	m_uncheckAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::onUncheckAll ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_EXCHANGE_FOOTPRINTS_BASE::OnOKClicked ), NULL, this );

}
