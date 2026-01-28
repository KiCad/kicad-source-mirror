///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "panel_assign_component_classes_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_ASSIGN_COMPONENT_CLASSES_BASE::PANEL_ASSIGN_COMPONENT_CLASSES_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_assignSheetClasses = new wxCheckBox( this, wxID_ANY, _("Assign component class per sheet"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_assignSheetClasses, 0, wxALL, 10 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizer12;
	bSizer12 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Custom Assignments:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer12->Add( m_staticText3, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizer12->Add( 0, 0, 1, wxBOTTOM|wxEXPAND, 5 );

	m_btnAddAssignment = new wxButton( this, wxID_ANY, _("Add Custom Assignment"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer12->Add( m_btnAddAssignment, 0, 0, 5 );


	bSizerMain->Add( bSizer12, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	m_assignmentsScrollWindow = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_assignmentsScrollWindow->SetScrollRate( 5, 5 );
	wxBoxSizer* m_assignmentsList;
	m_assignmentsList = new wxBoxSizer( wxVERTICAL );


	m_assignmentsScrollWindow->SetSizer( m_assignmentsList );
	m_assignmentsScrollWindow->Layout();
	m_assignmentsList->Fit( m_assignmentsScrollWindow );
	bSizerMain->Add( m_assignmentsScrollWindow, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_btnAddAssignment->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ASSIGN_COMPONENT_CLASSES_BASE::OnAddAssignmentClick ), NULL, this );
}

PANEL_ASSIGN_COMPONENT_CLASSES_BASE::~PANEL_ASSIGN_COMPONENT_CLASSES_BASE()
{
	// Disconnect Events
	m_btnAddAssignment->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_ASSIGN_COMPONENT_CLASSES_BASE::OnAddAssignmentClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxStaticBoxSizer* m_assignmentGroup;
	m_assignmentGroup = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxEmptyString ), wxVERTICAL );

	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxHORIZONTAL );

	m_componentClassLabel = new wxStaticText( this, wxID_ANY, _("Component class:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_componentClassLabel->Wrap( -1 );
	bSizer13->Add( m_componentClassLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_componentClass = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer13->Add( m_componentClass, 1, wxEXPAND, 5 );


	bSizer13->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonDeleteAssignment = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteAssignment->SetToolTip( _("Delete row") );

	bSizer13->Add( m_buttonDeleteAssignment, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_assignmentGroup->Add( bSizer13, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_buttonAddCondition = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizer3->Add( m_buttonAddCondition, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	bSizer3->Add( 30, 0, 1, wxEXPAND, 5 );

	m_buttonHighlightItems = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonHighlightItems->SetToolTip( _("Highlight matching footprints") );

	bSizer3->Add( m_buttonHighlightItems, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	bSizer3->Add( m_staticline2, 0, wxALL|wxEXPAND, 5 );


	bSizer3->Add( 5, 0, 0, wxEXPAND, 5 );

	m_radioAll = new wxRadioButton( this, wxID_ANY, _("Match all"), wxDefaultPosition, wxDefaultSize, 0 );
	m_radioAll->SetValue( true );
	bSizer3->Add( m_radioAll, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_radioAny = new wxRadioButton( this, wxID_ANY, _("Match any"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_radioAny, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_assignmentGroup->Add( bSizer3, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( m_assignmentGroup );
	this->Layout();
	m_assignmentGroup->Fit( this );

	// Connect Events
	m_buttonDeleteAssignment->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::OnDeleteAssignmentClick ), NULL, this );
	m_buttonAddCondition->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::OnAddConditionClick ), NULL, this );
	m_buttonHighlightItems->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::OnHighlightItemsClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::~PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE()
{
	// Disconnect Events
	m_buttonDeleteAssignment->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::OnDeleteAssignmentClick ), NULL, this );
	m_buttonAddCondition->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::OnAddConditionClick ), NULL, this );
	m_buttonHighlightItems->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE::OnHighlightItemsClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 5, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Reference:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_refs = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_conditionSizer->Add( m_refs, 0, wxEXPAND|wxLEFT, 5 );

	m_buttonImportRefs = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonImportRefs->SetToolTip( _("Import references") );

	m_conditionSizer->Add( m_buttonImportRefs, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	m_conditionSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer5->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer5 );
	this->Layout();
	bSizer5->Fit( this );

	// Connect Events
	m_refs->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::OnReferenceRightDown ), NULL, this );
	m_buttonImportRefs->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::OnImportRefsClick ), NULL, this );
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::~PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE()
{
	// Disconnect Events
	m_refs->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::OnReferenceRightDown ), NULL, this );
	m_buttonImportRefs->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::OnImportRefsClick ), NULL, this );
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE::OnDeleteConditionClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE::PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 4, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Side:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_side = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_conditionSizer->Add( m_side, 0, wxEXPAND|wxLEFT, 5 );


	m_conditionSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizer6->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();
	bSizer6->Fit( this );

	// Connect Events
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE::~PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE()
{
	// Disconnect Events
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE::OnDeleteConditionClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE::PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 5, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_rotation = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_conditionSizer->Add( m_rotation, 0, wxEXPAND|wxLEFT, 5 );

	m_rotUnit = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rotUnit->Wrap( -1 );
	m_conditionSizer->Add( m_rotUnit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	m_conditionSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer7->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );

	// Connect Events
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE::~PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE()
{
	// Disconnect Events
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE::OnDeleteConditionClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE::PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 6, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Footprint:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_footprint = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_conditionSizer->Add( m_footprint, 0, wxEXPAND|wxLEFT, 5 );

	m_buttonShowLibrary = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonShowLibrary->SetToolTip( _("Show library") );

	m_conditionSizer->Add( m_buttonShowLibrary, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	m_conditionSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer8->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer8 );
	this->Layout();
	bSizer8->Fit( this );

	// Connect Events
	m_buttonShowLibrary->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE::OnShowLibraryClick ), NULL, this );
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE::~PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE()
{
	// Disconnect Events
	m_buttonShowLibrary->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE::OnShowLibraryClick ), NULL, this );
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE::OnDeleteConditionClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE::PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 7, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->AddGrowableCol( 4 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Footprint field:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_fieldName = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_fieldName->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_fieldName, 0, wxEXPAND|wxLEFT, 5 );


	m_conditionSizer->Add( 10, 0, 1, wxEXPAND, 5 );

	m_staticText44 = new wxStaticText( this, wxID_ANY, _("Field value:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText44->Wrap( -1 );
	m_staticText44->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_staticText44, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_fieldValue = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fieldValue->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_fieldValue, 0, wxEXPAND|wxLEFT, 5 );


	m_conditionSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer9->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer9 );
	this->Layout();
	bSizer9->Fit( this );

	// Connect Events
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE::~PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE()
{
	// Disconnect Events
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE::OnDeleteConditionClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE::PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 4, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Custom:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_customCondition = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_conditionSizer->Add( m_customCondition, 0, wxEXPAND|wxLEFT, 5 );


	m_conditionSizer->Add( 20, 0, 0, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer10->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer10 );
	this->Layout();
	bSizer10->Fit( this );

	// Connect Events
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE::~PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE()
{
	// Disconnect Events
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE::OnDeleteConditionClick ), NULL, this );

}

PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE::PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DLIGHT ) );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* m_conditionSizer;
	m_conditionSizer = new wxFlexGridSizer( 0, 4, 0, 0 );
	m_conditionSizer->AddGrowableCol( 1 );
	m_conditionSizer->SetFlexibleDirection( wxBOTH );
	m_conditionSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_title = new wxStaticText( this, wxID_ANY, _("Sheet name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_title->Wrap( -1 );
	m_title->SetFont( wxFont( 12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Helvetica") ) );

	m_conditionSizer->Add( m_title, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_sheetName = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	m_conditionSizer->Add( m_sheetName, 0, wxEXPAND, 5 );


	m_conditionSizer->Add( 20, 0, 1, wxEXPAND, 5 );

	m_buttonDeleteMatch = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_buttonDeleteMatch->SetToolTip( _("Delete row") );

	m_conditionSizer->Add( m_buttonDeleteMatch, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizer11->Add( m_conditionSizer, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizer11 );
	this->Layout();
	bSizer11->Fit( this );

	// Connect Events
	m_buttonDeleteMatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE::OnDeleteConditionClick ), NULL, this );
}

PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE::~PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE()
{
	// Disconnect Events
	m_buttonDeleteMatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE::OnDeleteConditionClick ), NULL, this );

}
