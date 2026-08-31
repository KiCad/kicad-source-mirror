///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_drill_groups_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_DRILL_GROUPS_BASE::DIALOG_DRILL_GROUPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMain;
	bMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bProfile;
	bProfile = new wxBoxSizer( wxHORIZONTAL );

	m_profileLabel = new wxStaticText( this, wxID_ANY, _("Profile:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_profileLabel->Wrap( -1 );
	bProfile->Add( m_profileLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_profileName = new wxStaticText( this, wxID_ANY, _("-"), wxDefaultPosition, wxDefaultSize, 0 );
	m_profileName->Wrap( -1 );
	bProfile->Add( m_profileName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_freezeAssignments = new wxCheckBox( this, wxID_ANY, _("Freeze assignments"), wxDefaultPosition, wxDefaultSize, 0 );
	m_freezeAssignments->SetValue(true);
	bProfile->Add( m_freezeAssignments, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bMain->Add( bProfile, 0, wxEXPAND|wxALL, 5 );

	m_groupGrid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_groupGrid->CreateGrid( 0, 7 );
	m_groupGrid->EnableEditing( true );
	m_groupGrid->EnableGridLines( true );
	m_groupGrid->SetGridLineColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );
	m_groupGrid->EnableDragGridSize( false );
	m_groupGrid->SetMargins( 0, 0 );

	// Columns
	m_groupGrid->EnableDragColMove( false );
	m_groupGrid->EnableDragColSize( true );
	m_groupGrid->SetColLabelSize( 22 );
	m_groupGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_groupGrid->EnableDragRowSize( false );
	m_groupGrid->SetRowLabelSize( 0 );
	m_groupGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_groupGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bMain->Add( m_groupGrid, 1, wxEXPAND|wxALL, 5 );

	wxStaticBoxSizer* sbDetail;
	sbDetail = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Selected group") ), wxVERTICAL );

	wxFlexGridSizer* fgDetail;
	fgDetail = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgDetail->AddGrowableCol( 1 );
	fgDetail->SetFlexibleDirection( wxBOTH );
	fgDetail->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_markLabel = new wxStaticText( sbDetail->GetStaticBox(), wxID_ANY, _("Mark:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_markLabel->Wrap( -1 );
	fgDetail->Add( m_markLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_markCtrlChoices[] = { _("Shape"), _("Letter"), _("Hole size text") };
	int m_markCtrlNChoices = sizeof( m_markCtrlChoices ) / sizeof( wxString );
	m_markCtrl = new wxChoice( sbDetail->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_markCtrlNChoices, m_markCtrlChoices, 0 );
	m_markCtrl->SetSelection( 0 );
	fgDetail->Add( m_markCtrl, 0, wxEXPAND, 5 );

	m_shapeLabel = new wxStaticText( sbDetail->GetStaticBox(), wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_shapeLabel->Wrap( -1 );
	fgDetail->Add( m_shapeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_shapeCtrl = new wxSpinCtrl( sbDetail->GetStaticBox(), wxID_ANY, wxT("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 63, 0 );
	fgDetail->Add( m_shapeCtrl, 0, wxEXPAND, 5 );

	m_letterLabel = new wxStaticText( sbDetail->GetStaticBox(), wxID_ANY, _("Letter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_letterLabel->Wrap( -1 );
	fgDetail->Add( m_letterLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_letterCtrl = new wxTextCtrl( sbDetail->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgDetail->Add( m_letterCtrl, 0, wxEXPAND, 5 );

	m_descriptionLabel = new wxStaticText( sbDetail->GetStaticBox(), wxID_ANY, _("Description:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_descriptionLabel->Wrap( -1 );
	fgDetail->Add( m_descriptionLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_descriptionCtrl = new wxTextCtrl( sbDetail->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgDetail->Add( m_descriptionCtrl, 0, wxEXPAND, 5 );


	sbDetail->Add( fgDetail, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bGroupButtons;
	bGroupButtons = new wxBoxSizer( wxHORIZONTAL );

	m_flashButton = new wxButton( sbDetail->GetStaticBox(), wxID_ANY, _("Flash on Canvas"), wxDefaultPosition, wxDefaultSize, 0 );
	bGroupButtons->Add( m_flashButton, 0, wxRIGHT, 5 );


	sbDetail->Add( bGroupButtons, 0, wxEXPAND|wxALL, 5 );


	bMain->Add( sbDetail, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bBottom;
	bBottom = new wxBoxSizer( wxHORIZONTAL );

	m_closeButton = new wxButton( this, wxID_ANY, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bBottom->Add( m_closeButton, 0, wxRIGHT, 5 );


	bMain->Add( bBottom, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_freezeAssignments->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRILL_GROUPS_BASE::onFreezeChanged ), NULL, this );
	m_flashButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRILL_GROUPS_BASE::onFlash ), NULL, this );
	m_closeButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRILL_GROUPS_BASE::onClose ), NULL, this );
}

DIALOG_DRILL_GROUPS_BASE::~DIALOG_DRILL_GROUPS_BASE()
{
	// Disconnect Events
	m_freezeAssignments->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_DRILL_GROUPS_BASE::onFreezeChanged ), NULL, this );
	m_flashButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRILL_GROUPS_BASE::onFlash ), NULL, this );
	m_closeButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_DRILL_GROUPS_BASE::onClose ), NULL, this );

}
