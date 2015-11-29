///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_create_array_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_CREATE_ARRAY_BASE::DIALOG_CREATE_ARRAY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_gridTypeNotebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_gridPanel = new wxPanel( m_gridTypeNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_labelNx = new wxStaticText( m_gridPanel, wxID_ANY, _("Horizontal count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelNx->Wrap( -1 );
	gbSizer1->Add( m_labelNx, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_RIGHT|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryNx = new wxTextCtrl( m_gridPanel, wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryNx, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_labelNy = new wxStaticText( m_gridPanel, wxID_ANY, _("Vertical count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelNy->Wrap( -1 );
	gbSizer1->Add( m_labelNy, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_RIGHT|wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryNy = new wxTextCtrl( m_gridPanel, wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryNy, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_labelDx = new wxStaticText( m_gridPanel, wxID_ANY, _("Horizontal spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDx->Wrap( -1 );
	gbSizer1->Add( m_labelDx, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryDx = new wxTextCtrl( m_gridPanel, wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryDx, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelDx = new wxStaticText( m_gridPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelDx->Wrap( -1 );
	gbSizer1->Add( m_unitLabelDx, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelDy = new wxStaticText( m_gridPanel, wxID_ANY, _("Vertical spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDy->Wrap( -1 );
	gbSizer1->Add( m_labelDy, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryDy = new wxTextCtrl( m_gridPanel, wxID_ANY, _("5"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryDy, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelDy = new wxStaticText( m_gridPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelDy->Wrap( -1 );
	gbSizer1->Add( m_unitLabelDy, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelOffsetX = new wxStaticText( m_gridPanel, wxID_ANY, _("Horizontal offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelOffsetX->Wrap( -1 );
	gbSizer1->Add( m_labelOffsetX, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryOffsetX = new wxTextCtrl( m_gridPanel, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryOffsetX, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelOffsetX = new wxStaticText( m_gridPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelOffsetX->Wrap( -1 );
	gbSizer1->Add( m_unitLabelOffsetX, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelOffsetY = new wxStaticText( m_gridPanel, wxID_ANY, _("Vertical offset:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelOffsetY->Wrap( -1 );
	gbSizer1->Add( m_labelOffsetY, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryOffsetY = new wxTextCtrl( m_gridPanel, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryOffsetY, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelOffsetY = new wxStaticText( m_gridPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelOffsetY->Wrap( -1 );
	gbSizer1->Add( m_unitLabelOffsetY, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelStagger = new wxStaticText( m_gridPanel, wxID_ANY, _("Stagger:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelStagger->Wrap( -1 );
	gbSizer1->Add( m_labelStagger, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_entryStagger = new wxTextCtrl( m_gridPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_entryStagger, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	wxString m_radioBoxGridStaggerTypeChoices[] = { _("Rows"), _("Columns") };
	int m_radioBoxGridStaggerTypeNChoices = sizeof( m_radioBoxGridStaggerTypeChoices ) / sizeof( wxString );
	m_radioBoxGridStaggerType = new wxRadioBox( m_gridPanel, wxID_ANY, _("Stagger Type"), wxDefaultPosition, wxDefaultSize, m_radioBoxGridStaggerTypeNChoices, m_radioBoxGridStaggerTypeChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxGridStaggerType->SetSelection( 1 );
	gbSizer1->Add( m_radioBoxGridStaggerType, wxGBPosition( 7, 0 ), wxGBSpan( 2, 2 ), wxALL|wxEXPAND, 5 );
	
	
	bSizer2->Add( gbSizer1, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );
	
	wxString m_radioBoxGridNumberingAxisChoices[] = { _("Horizontal, then vertical"), _("Vertical, then horizontal") };
	int m_radioBoxGridNumberingAxisNChoices = sizeof( m_radioBoxGridNumberingAxisChoices ) / sizeof( wxString );
	m_radioBoxGridNumberingAxis = new wxRadioBox( m_gridPanel, wxID_ANY, _("Numbering Direction"), wxDefaultPosition, wxDefaultSize, m_radioBoxGridNumberingAxisNChoices, m_radioBoxGridNumberingAxisChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxGridNumberingAxis->SetSelection( 0 );
	bSizer3->Add( m_radioBoxGridNumberingAxis, 0, wxALL|wxEXPAND, 5 );
	
	m_checkBoxGridReverseNumbering = new wxCheckBox( m_gridPanel, wxID_ANY, _("Reverse numbering on alternate rows or columns"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_checkBoxGridReverseNumbering, 0, wxALL, 5 );
	
	m_checkBoxGridRestartNumbering = new wxCheckBox( m_gridPanel, wxID_ANY, _("Restart numbering"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxGridRestartNumbering->SetValue(true); 
	bSizer3->Add( m_checkBoxGridRestartNumbering, 0, wxALL, 5 );
	
	wxString m_radioBoxGridNumberingSchemeChoices[] = { _("Continuous (1, 2, 3...)"), _("Coordinate (A1, A2, ... B1, ...)") };
	int m_radioBoxGridNumberingSchemeNChoices = sizeof( m_radioBoxGridNumberingSchemeChoices ) / sizeof( wxString );
	m_radioBoxGridNumberingScheme = new wxRadioBox( m_gridPanel, wxID_ANY, _("Numbering Scheme"), wxDefaultPosition, wxDefaultSize, m_radioBoxGridNumberingSchemeNChoices, m_radioBoxGridNumberingSchemeChoices, 1, wxRA_SPECIFY_COLS );
	m_radioBoxGridNumberingScheme->SetSelection( 1 );
	bSizer3->Add( m_radioBoxGridNumberingScheme, 0, wxALL|wxEXPAND, 5 );
	
	m_labelPriAxisNumbering = new wxStaticText( m_gridPanel, wxID_ANY, _("Primary axis numbering:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelPriAxisNumbering->Wrap( -1 );
	bSizer3->Add( m_labelPriAxisNumbering, 0, wxLEFT|wxRIGHT|wxTOP, 5 );
	
	wxArrayString m_choicePriAxisNumberingChoices;
	m_choicePriAxisNumbering = new wxChoice( m_gridPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choicePriAxisNumberingChoices, 0 );
	m_choicePriAxisNumbering->SetSelection( 0 );
	bSizer3->Add( m_choicePriAxisNumbering, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_labelSecAxisNumbering = new wxStaticText( m_gridPanel, wxID_ANY, _("Secondary axis numbering:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelSecAxisNumbering->Wrap( -1 );
	m_labelSecAxisNumbering->Enable( false );
	
	bSizer3->Add( m_labelSecAxisNumbering, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_choiceSecAxisNumberingChoices;
	m_choiceSecAxisNumbering = new wxChoice( m_gridPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceSecAxisNumberingChoices, 0 );
	m_choiceSecAxisNumbering->SetSelection( 0 );
	m_choiceSecAxisNumbering->Enable( false );
	
	bSizer3->Add( m_choiceSecAxisNumbering, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	m_labelGridNumberingOffset = new wxStaticText( m_gridPanel, wxID_ANY, _("Numbering start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelGridNumberingOffset->Wrap( -1 );
	bSizer5->Add( m_labelGridNumberingOffset, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_entryGridPriNumberingOffset = new wxTextCtrl( m_gridPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_entryGridPriNumberingOffset, 0, wxALL, 5 );
	
	m_entryGridSecNumberingOffset = new wxTextCtrl( m_gridPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_entryGridSecNumberingOffset, 0, wxALL, 5 );
	
	
	bSizer3->Add( bSizer5, 0, wxEXPAND, 5 );
	
	
	bSizer2->Add( bSizer3, 0, wxALL|wxEXPAND, 5 );
	
	
	m_gridPanel->SetSizer( bSizer2 );
	m_gridPanel->Layout();
	bSizer2->Fit( m_gridPanel );
	m_gridTypeNotebook->AddPage( m_gridPanel, _("Grid Array"), true );
	m_circularPanel = new wxPanel( m_gridTypeNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );
	
	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 0, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_labelCentreX = new wxStaticText( m_circularPanel, wxID_ANY, _("Horizontal center:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCentreX->Wrap( -1 );
	gbSizer2->Add( m_labelCentreX, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_entryCentreX = new wxTextCtrl( m_circularPanel, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_entryCentreX, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelCentreX = new wxStaticText( m_circularPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCentreX->Wrap( -1 );
	gbSizer2->Add( m_unitLabelCentreX, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelCentreY = new wxStaticText( m_circularPanel, wxID_ANY, _("Vertical center:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCentreY->Wrap( -1 );
	gbSizer2->Add( m_labelCentreY, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_entryCentreY = new wxTextCtrl( m_circularPanel, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_entryCentreY, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelCentreY = new wxStaticText( m_circularPanel, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCentreY->Wrap( -1 );
	gbSizer2->Add( m_unitLabelCentreY, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelCircRadius = new wxStaticText( m_circularPanel, wxID_ANY, _("Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircRadius->Wrap( -1 );
	gbSizer2->Add( m_labelCircRadius, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_labelCircRadiusValue = new wxStaticText( m_circularPanel, wxID_ANY, _("0 mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircRadiusValue->Wrap( -1 );
	gbSizer2->Add( m_labelCircRadiusValue, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_labelCircAngle = new wxStaticText( m_circularPanel, wxID_ANY, _("Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircAngle->Wrap( -1 );
	gbSizer2->Add( m_labelCircAngle, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_entryCircAngle = new wxTextCtrl( m_circularPanel, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryCircAngle->SetToolTip( _("Positive angles represent an anti-clockwise rotation. An angle of 0 will produce a full circle divided evenly into \"Count\" portions.") );
	
	gbSizer2->Add( m_entryCircAngle, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_unitLabelCircAngle = new wxStaticText( m_circularPanel, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitLabelCircAngle->Wrap( -1 );
	gbSizer2->Add( m_unitLabelCircAngle, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_labelCircCount = new wxStaticText( m_circularPanel, wxID_ANY, _("Count:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircCount->Wrap( -1 );
	gbSizer2->Add( m_labelCircCount, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_entryCircCount = new wxTextCtrl( m_circularPanel, wxID_ANY, _("4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_entryCircCount->SetToolTip( _("How many items in the array.") );
	
	gbSizer2->Add( m_entryCircCount, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	m_labelCircRotate = new wxStaticText( m_circularPanel, wxID_ANY, _("Rotate:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircRotate->Wrap( -1 );
	gbSizer2->Add( m_labelCircRotate, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALL|wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT, 5 );
	
	m_entryRotateItemsCb = new wxCheckBox( m_circularPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_entryRotateItemsCb->SetValue(true); 
	m_entryRotateItemsCb->SetToolTip( _("Rotate the item as well as move it - multi-selections will be rotated together") );
	
	gbSizer2->Add( m_entryRotateItemsCb, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );
	
	
	bSizer4->Add( gbSizer2, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_checkBoxCircRestartNumbering = new wxCheckBox( m_circularPanel, wxID_ANY, _("Restart numbering"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBoxCircRestartNumbering->SetValue(true); 
	bSizer6->Add( m_checkBoxCircRestartNumbering, 0, wxALL, 5 );
	
	m_labelCircNumbering = new wxStaticText( m_circularPanel, wxID_ANY, _("Numbering type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircNumbering->Wrap( -1 );
	bSizer6->Add( m_labelCircNumbering, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_choiceCircNumberingTypeChoices;
	m_choiceCircNumberingType = new wxChoice( m_circularPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceCircNumberingTypeChoices, 0 );
	m_choiceCircNumberingType->SetSelection( 0 );
	bSizer6->Add( m_choiceCircNumberingType, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	m_labelCircNumStart = new wxStaticText( m_circularPanel, wxID_ANY, _("Numbering start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelCircNumStart->Wrap( -1 );
	bSizer7->Add( m_labelCircNumStart, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_entryCircNumberingStart = new wxTextCtrl( m_circularPanel, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_entryCircNumberingStart, 0, wxALL, 5 );
	
	
	bSizer6->Add( bSizer7, 0, wxEXPAND, 5 );
	
	
	bSizer4->Add( bSizer6, 1, wxALL|wxEXPAND, 5 );
	
	
	m_circularPanel->SetSizer( bSizer4 );
	m_circularPanel->Layout();
	bSizer4->Fit( m_circularPanel );
	m_gridTypeNotebook->AddPage( m_circularPanel, _("Circular Array"), false );
	
	bMainSizer->Add( m_gridTypeNotebook, 1, wxEXPAND | wxALL, 5 );
	
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
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CREATE_ARRAY_BASE::OnClose ) );
	m_entryNx->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryNy->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDx->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDy->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetX->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetY->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryStagger->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_checkBoxGridRestartNumbering->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_radioBoxGridNumberingScheme->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCentreX->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCentreY->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircAngle->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircCount->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_stdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnOkClick ), NULL, this );
}

DIALOG_CREATE_ARRAY_BASE::~DIALOG_CREATE_ARRAY_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_CREATE_ARRAY_BASE::OnClose ) );
	m_entryNx->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryNy->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDx->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryDy->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetX->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryOffsetY->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryStagger->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_checkBoxGridRestartNumbering->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_radioBoxGridNumberingScheme->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCentreX->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCentreY->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircAngle->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_entryCircCount->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnParameterChanged ), NULL, this );
	m_stdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_CREATE_ARRAY_BASE::OnOkClick ), NULL, this );
	
}
