///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_griditem_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GRIDITEM_PROPERTIES_BASE::DIALOG_GRIDITEM_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_notebookGridDefs = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_rectangleByCorners = new wxPanel( m_notebookGridDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer18;
	bSizer18 = new wxBoxSizer( wxVERTICAL );

	m_gbsRectangleByCorners = new wxGridBagSizer( 4, 5 );
	m_gbsRectangleByCorners->SetFlexibleDirection( wxBOTH );
	m_gbsRectangleByCorners->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer18->Add( m_gbsRectangleByCorners, 1, wxALL|wxEXPAND, 5 );


	m_rectangleByCorners->SetSizer( bSizer18 );
	m_rectangleByCorners->Layout();
	bSizer18->Fit( m_rectangleByCorners );
	m_notebookGridDefs->AddPage( m_rectangleByCorners, _("By Corners"), false );
	m_rectangleByCornerSize = new wxPanel( m_notebookGridDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxVERTICAL );

	m_gbsRectangleByCornerSize = new wxGridBagSizer( 4, 5 );
	m_gbsRectangleByCornerSize->SetFlexibleDirection( wxBOTH );
	m_gbsRectangleByCornerSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer19->Add( m_gbsRectangleByCornerSize, 1, wxALL|wxEXPAND, 5 );


	m_rectangleByCornerSize->SetSizer( bSizer19 );
	m_rectangleByCornerSize->Layout();
	bSizer19->Fit( m_rectangleByCornerSize );
	m_notebookGridDefs->AddPage( m_rectangleByCornerSize, _("By Corner and Size"), false );
	m_rectangleByCenterSize = new wxPanel( m_notebookGridDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );

	m_gbsRectangleByCenterSize = new wxGridBagSizer( 4, 5 );
	m_gbsRectangleByCenterSize->SetFlexibleDirection( wxBOTH );
	m_gbsRectangleByCenterSize->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer20->Add( m_gbsRectangleByCenterSize, 1, wxALL|wxEXPAND, 5 );


	m_rectangleByCenterSize->SetSizer( bSizer20 );
	m_rectangleByCenterSize->Layout();
	bSizer20->Fit( m_rectangleByCenterSize );
	m_notebookGridDefs->AddPage( m_rectangleByCenterSize, _("By Center and Size"), true );
	m_polarCenterRadius = new wxPanel( m_notebookGridDefs, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	m_gbsPolarCenterRadius = new wxGridBagSizer( 4, 5 );
	m_gbsPolarCenterRadius->SetFlexibleDirection( wxBOTH );
	m_gbsPolarCenterRadius->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	bSizer21->Add( m_gbsPolarCenterRadius, 1, wxALL|wxEXPAND, 5 );


	m_polarCenterRadius->SetSizer( bSizer21 );
	m_polarCenterRadius->Layout();
	bSizer21->Fit( m_polarCenterRadius );
	m_notebookGridDefs->AddPage( m_polarCenterRadius, _("Center and Radius"), true );

	bMainSizer->Add( m_notebookGridDefs, 0, wxEXPAND | wxALL, 5 );

	m_locked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	bMainSizer->Add( m_locked, 0, wxTOP|wxRIGHT|wxLEFT, 10 );

	m_upperSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 2, 5 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer2->SetEmptyCellSize( wxSize( -1,6 ) );

	m_gridTypeLabel = new wxStaticText( this, wxID_ANY, _("Grid type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_gridTypeLabel->Wrap( -1 );
	gbSizer2->Add( m_gridTypeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxString m_gridTypeCtrlChoices[] = { _("Cartesian"), _("Polar") };
	int m_gridTypeCtrlNChoices = sizeof( m_gridTypeCtrlChoices ) / sizeof( wxString );
	m_gridTypeCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_gridTypeCtrlNChoices, m_gridTypeCtrlChoices, 0 );
	m_gridTypeCtrl->SetSelection( 0 );
	gbSizer2->Add( m_gridTypeCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_orientationLabel = new wxStaticText( this, wxID_ANY, _("Orientation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationLabel->Wrap( -1 );
	gbSizer2->Add( m_orientationLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_orientationCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_orientationCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_orientationUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_orientationUnits->Wrap( -1 );
	gbSizer2->Add( m_orientationUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spacingXLabel = new wxStaticText( this, wxID_ANY, _("Spacing X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingXLabel->Wrap( -1 );
	gbSizer2->Add( m_spacingXLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spacingXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingXCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_spacingXCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_spacingXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingXUnits->Wrap( -1 );
	gbSizer2->Add( m_spacingXUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_spacingYLabel = new wxStaticText( this, wxID_ANY, _("Spacing Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingYLabel->Wrap( -1 );
	gbSizer2->Add( m_spacingYLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_spacingYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingYCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_spacingYCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_spacingYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_spacingYUnits->Wrap( -1 );
	gbSizer2->Add( m_spacingYUnits, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_tickIntervalLabel = new wxStaticText( this, wxID_ANY, _("Major tick every:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tickIntervalLabel->Wrap( -1 );
	gbSizer2->Add( m_tickIntervalLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_tickIntervalCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_tickIntervalCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_tickIntervalCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_tickIntervalUnits = new wxStaticText( this, wxID_ANY, _("lines"), wxDefaultPosition, wxDefaultSize, 0 );
	m_tickIntervalUnits->Wrap( -1 );
	gbSizer2->Add( m_tickIntervalUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_priorityLabel = new wxStaticText( this, wxID_ANY, _("Priority:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_priorityLabel->Wrap( -1 );
	gbSizer2->Add( m_priorityLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_priorityCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_priorityCtrl->SetMinSize( wxSize( 140,-1 ) );

	gbSizer2->Add( m_priorityCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_affectsLabel = new wxStaticText( this, wxID_ANY, _("Affects:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_affectsLabel->Wrap( -1 );
	gbSizer2->Add( m_affectsLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxBoxSizer* bSizerAffects;
	bSizerAffects = new wxBoxSizer( wxHORIZONTAL );

	m_affectsCursor = new wxCheckBox( this, wxID_ANY, _("Cursor"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerAffects->Add( m_affectsCursor, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_affectsRouting = new wxCheckBox( this, wxID_ANY, _("Routing"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerAffects->Add( m_affectsRouting, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_affectsPlacement = new wxCheckBox( this, wxID_ANY, _("Placement"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerAffects->Add( m_affectsPlacement, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	gbSizer2->Add( bSizerAffects, wxGBPosition( 6, 1 ), wxGBSpan( 1, 2 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer2->AddGrowableCol( 1 );

	m_upperSizer->Add( gbSizer2, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bMainSizer->Add( m_upperSizer, 1, wxALL|wxEXPAND, 5 );

	m_StandardButtonsSizer = new wxStdDialogButtonSizer();
	m_StandardButtonsSizerOK = new wxButton( this, wxID_OK );
	m_StandardButtonsSizer->AddButton( m_StandardButtonsSizerOK );
	m_StandardButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_StandardButtonsSizer->AddButton( m_StandardButtonsSizerCancel );
	m_StandardButtonsSizer->Realize();

	bMainSizer->Add( m_StandardButtonsSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

DIALOG_GRIDITEM_PROPERTIES_BASE::~DIALOG_GRIDITEM_PROPERTIES_BASE()
{
}
