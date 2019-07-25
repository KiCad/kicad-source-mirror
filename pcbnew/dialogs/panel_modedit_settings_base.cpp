///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_modedit_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_MODEDIT_SETTINGS_BASE::PANEL_MODEDIT_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerColumns;
	bSizerColumns = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerDisplayOptions;
	bSizerDisplayOptions = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PolarDisplayChoices[] = { _("Cartesian coordinates"), _("Polar coordinates") };
	int m_PolarDisplayNChoices = sizeof( m_PolarDisplayChoices ) / sizeof( wxString );
	m_PolarDisplay = new wxRadioBox( this, wxID_POLAR_CTRL, _("Coordinates"), wxDefaultPosition, wxDefaultSize, m_PolarDisplayNChoices, m_PolarDisplayChoices, 1, wxRA_SPECIFY_COLS );
	m_PolarDisplay->SetSelection( 0 );
	m_PolarDisplay->SetToolTip( _("Set display of relative (dx/dy) coordinates to Cartesian (rectangular) or polar (angle/distance).") );
	
	bSizerDisplayOptions->Add( m_PolarDisplay, 0, wxEXPAND|wxBOTTOM, 5 );
	
	wxString m_UnitsSelectionChoices[] = { _("Inches"), _("Millimeters") };
	int m_UnitsSelectionNChoices = sizeof( m_UnitsSelectionChoices ) / sizeof( wxString );
	m_UnitsSelection = new wxRadioBox( this, wxID_UNITS, _("Units"), wxDefaultPosition, wxDefaultSize, m_UnitsSelectionNChoices, m_UnitsSelectionChoices, 1, wxRA_SPECIFY_COLS );
	m_UnitsSelection->SetSelection( 0 );
	m_UnitsSelection->SetToolTip( _("Set units used to display dimensions and positions.") );
	
	bSizerDisplayOptions->Add( m_UnitsSelection, 0, wxEXPAND|wxBOTTOM, 5 );
	
	
	bSizerColumns->Add( bSizerDisplayOptions, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerEditOptions;
	sbSizerEditOptions = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Editing Options") ), wxVERTICAL );
	
	m_MagneticPads = new wxCheckBox( sbSizerEditOptions->GetStaticBox(), wxID_ANY, _("Magnetic pads"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerEditOptions->Add( m_MagneticPads, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Segments_45_Only_Ctrl = new wxCheckBox( sbSizerEditOptions->GetStaticBox(), wxID_SEGMENTS45, _("L&imit graphic lines to H, V and 45 degrees"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Segments_45_Only_Ctrl->SetToolTip( _("Force line segment directions to H, V or 45 degrees when drawing on technical layers.") );
	
	sbSizerEditOptions->Add( m_Segments_45_Only_Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bSizerColumns->Add( sbSizerEditOptions, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	
	bSizerMain->Add( bSizerColumns, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
}

PANEL_MODEDIT_SETTINGS_BASE::~PANEL_MODEDIT_SETTINGS_BASE()
{
}
