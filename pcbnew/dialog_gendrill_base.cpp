///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_gendrill_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GENDRILL_BASE::DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_Choice_UnitChoices[] = { _("Millimeters"), _("Inches") };
	int m_Choice_UnitNChoices = sizeof( m_Choice_UnitChoices ) / sizeof( wxString );
	m_Choice_Unit = new wxRadioBox( this, wxID_ANY, _("Drill Units:"), wxDefaultPosition, wxDefaultSize, m_Choice_UnitNChoices, m_Choice_UnitChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Unit->SetSelection( 1 );
	m_LeftBoxSizer->Add( m_Choice_Unit, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_Zeros_FormatChoices[] = { _("Decimal format"), _("Suppress leading zeros"), _("Suppress trailing zeros"), _("Keep zeros") };
	int m_Choice_Zeros_FormatNChoices = sizeof( m_Choice_Zeros_FormatChoices ) / sizeof( wxString );
	m_Choice_Zeros_Format = new wxRadioBox( this, wxID_ANY, _("Zeros Format"), wxDefaultPosition, wxDefaultSize, m_Choice_Zeros_FormatNChoices, m_Choice_Zeros_FormatChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Zeros_Format->SetSelection( 0 );
	m_Choice_Zeros_Format->SetToolTip( _("Choose EXCELLON numbers notation") );
	
	m_LeftBoxSizer->Add( m_Choice_Zeros_Format, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_PrecisionChoices[] = { _("2:3"), _("2:4") };
	int m_Choice_PrecisionNChoices = sizeof( m_Choice_PrecisionChoices ) / sizeof( wxString );
	m_Choice_Precision = new wxRadioBox( this, wxID_ANY, _("Precision"), wxDefaultPosition, wxDefaultSize, m_Choice_PrecisionNChoices, m_Choice_PrecisionChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Precision->SetSelection( 1 );
	m_Choice_Precision->SetToolTip( _("Choose EXCELLON numbers precision") );
	
	m_LeftBoxSizer->Add( m_Choice_Precision, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_Drill_OffsetChoices[] = { _("Absolute"), _("Auxiliary axis") };
	int m_Choice_Drill_OffsetNChoices = sizeof( m_Choice_Drill_OffsetChoices ) / sizeof( wxString );
	m_Choice_Drill_Offset = new wxRadioBox( this, wxID_ANY, _("Drill Origin:"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_OffsetNChoices, m_Choice_Drill_OffsetChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Offset->SetSelection( 0 );
	m_Choice_Drill_Offset->SetToolTip( _("Choose the coordinate origin: absolute or relative to the auxiliray axis") );
	
	m_LeftBoxSizer->Add( m_Choice_Drill_Offset, 0, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( m_LeftBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleBoxSizer;
	bMiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_Choice_Drill_MapChoices[] = { _("None"), _("Drill map (HPGL)"), _("Drill map (PostScript)"), _("Drill map (Gerber)"), _("Drill map (DXF)") };
	int m_Choice_Drill_MapNChoices = sizeof( m_Choice_Drill_MapChoices ) / sizeof( wxString );
	m_Choice_Drill_Map = new wxRadioBox( this, wxID_ANY, _("Drill Sheet:"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_MapNChoices, m_Choice_Drill_MapChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Map->SetSelection( 0 );
	m_Choice_Drill_Map->SetToolTip( _("Creates a drill map in PS, HPGL or other formats") );
	
	bMiddleBoxSizer->Add( m_Choice_Drill_Map, 0, wxALL|wxEXPAND, 5 );
	
	wxString m_Choice_Drill_ReportChoices[] = { _("None"), _("Drill report") };
	int m_Choice_Drill_ReportNChoices = sizeof( m_Choice_Drill_ReportChoices ) / sizeof( wxString );
	m_Choice_Drill_Report = new wxRadioBox( this, wxID_ANY, _("Drill Report:"), wxDefaultPosition, wxDefaultSize, m_Choice_Drill_ReportNChoices, m_Choice_Drill_ReportChoices, 1, wxRA_SPECIFY_COLS );
	m_Choice_Drill_Report->SetSelection( 0 );
	m_Choice_Drill_Report->SetToolTip( _("Creates a plain text report") );
	
	bMiddleBoxSizer->Add( m_Choice_Drill_Report, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbHPGOptionsSizer;
	sbHPGOptionsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("HPGL plotter Options:") ), wxVERTICAL );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Speed (cm/s)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	sbHPGOptionsSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PenSpeed = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbHPGOptionsSizer->Add( m_PenSpeed, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Pen Number"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	sbHPGOptionsSizer->Add( m_staticText2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PenNum = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbHPGOptionsSizer->Add( m_PenNum, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMiddleBoxSizer->Add( sbHPGOptionsSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbOptSizer;
	sbOptSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options:") ), wxVERTICAL );
	
	m_Check_Mirror = new wxCheckBox( this, wxID_ANY, _("Mirror y axis"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbOptSizer->Add( m_Check_Mirror, 0, wxALL, 5 );
	
	m_Check_Minimal = new wxCheckBox( this, wxID_ANY, _("Minimal header"), wxDefaultPosition, wxDefaultSize, 0 );
	
	sbOptSizer->Add( m_Check_Minimal, 0, wxALL, 5 );
	
	bMiddleBoxSizer->Add( sbOptSizer, 0, wxEXPAND, 5 );
	
	bMainSizer->Add( bMiddleBoxSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightBoxSizer;
	bRightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizerInfo;
	sbSizerInfo = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Info:") ), wxVERTICAL );
	
	m_DefaultViasDrillSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Default Vias Drill:") ), wxVERTICAL );
	
	m_ViaDrillValue = new wxStaticText( this, wxID_ANY, _("Via Drill Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillValue->Wrap( -1 );
	m_DefaultViasDrillSizer->Add( m_ViaDrillValue, 0, wxALL, 5 );
	
	sbSizerInfo->Add( m_DefaultViasDrillSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_MicroViasDrillSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Micro Vias Drill:") ), wxVERTICAL );
	
	m_MicroViaDrillValue = new wxStaticText( this, wxID_ANY, _("Micro Via Drill Value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaDrillValue->Wrap( -1 );
	m_MicroViasDrillSizer->Add( m_MicroViaDrillValue, 0, wxALL, 5 );
	
	sbSizerInfo->Add( m_MicroViasDrillSizer, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	wxStaticBoxSizer* sbSizerHoles;
	sbSizerHoles = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Holes Count:") ), wxVERTICAL );
	
	m_PadsCountInfoMsg = new wxStaticText( this, wxID_ANY, _("Pads:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadsCountInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_PadsCountInfoMsg, 0, wxALL, 5 );
	
	m_ThroughViasInfoMsg = new wxStaticText( this, wxID_ANY, _("Through Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThroughViasInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_ThroughViasInfoMsg, 0, wxALL, 5 );
	
	m_MicroViasInfoMsg = new wxStaticText( this, wxID_ANY, _("Micro Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViasInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_MicroViasInfoMsg, 0, wxALL, 5 );
	
	m_BuriedViasInfoMsg = new wxStaticText( this, wxID_ANY, _("Buried Vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BuriedViasInfoMsg->Wrap( -1 );
	sbSizerHoles->Add( m_BuriedViasInfoMsg, 0, wxALL, 5 );
	
	sbSizerInfo->Add( sbSizerHoles, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	bRightBoxSizer->Add( sbSizerInfo, 0, wxEXPAND|wxTOP, 5 );
	
	
	bRightBoxSizer->Add( 10, 10, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_OkButton = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OkButton->SetDefault(); 
	bRightBoxSizer->Add( m_OkButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_CancelButton = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightBoxSizer->Add( m_CancelButton, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	bMainSizer->Add( bRightBoxSizer, 1, wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	m_Choice_Unit->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelDrillUnitsSelected ), NULL, this );
	m_Choice_Zeros_Format->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelZerosFmtSelected ), NULL, this );
	m_OkButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnOkClick ), NULL, this );
	m_CancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnCancelClick ), NULL, this );
}

DIALOG_GENDRILL_BASE::~DIALOG_GENDRILL_BASE()
{
	// Disconnect Events
	m_Choice_Unit->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelDrillUnitsSelected ), NULL, this );
	m_Choice_Zeros_Format->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnSelZerosFmtSelected ), NULL, this );
	m_OkButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnOkClick ), NULL, this );
	m_CancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_GENDRILL_BASE::OnCancelClick ), NULL, this );
}
