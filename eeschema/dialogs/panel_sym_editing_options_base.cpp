///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_sym_editing_options_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SYM_EDITING_OPTIONS_BASE::PANEL_SYM_EDITING_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : RESETTABLE_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* p1mainSizer;
	p1mainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* leftColumn;
	leftColumn = new wxBoxSizer( wxVERTICAL );

	m_defaultsLabel = new wxStaticText( this, wxID_ANY, _("Defaults for New Objects"), wxDefaultPosition, wxDefaultSize, 0 );
	m_defaultsLabel->Wrap( -1 );
	leftColumn->Add( m_defaultsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	leftColumn->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	leftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,8 ) );

	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("&Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_lineWidthLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_lineWidthCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_lineWidthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	gbSizer1->Add( m_lineWidthUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_widthHelpText = new wxStaticText( this, wxID_ANY, _("Set to 0 to allow symbols to inherit line width properties\nfrom schematic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_widthHelpText->Wrap( -1 );
	gbSizer1->Add( m_widthHelpText, wxGBPosition( 1, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM|wxLEFT, 5 );

	m_textSizeLabel = new wxStaticText( this, wxID_ANY, _("Default text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_textSizeLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_textSizeCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_textSizeUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP, 5 );

	m_pinLengthLabel = new wxStaticText( this, wxID_ANY, _("D&efault pin length:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthLabel->Wrap( -1 );
	gbSizer1->Add( m_pinLengthLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinLengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_pinLengthCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinLengthUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinLengthUnits->Wrap( -1 );
	gbSizer1->Add( m_pinLengthUnits, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinNumSizeLabel = new wxStaticText( this, wxID_ANY, _("De&fault pin number size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_pinNumSizeLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinNumSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_pinNumSizeCtrl, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinNumSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNumSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_pinNumSizeUnits, wxGBPosition( 5, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pinNameSizeLabel = new wxStaticText( this, wxID_ANY, _("Def&ault pin name size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameSizeLabel->Wrap( -1 );
	gbSizer1->Add( m_pinNameSizeLabel, wxGBPosition( 6, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinNameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_pinNameSizeCtrl, wxGBPosition( 6, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinNameSizeUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinNameSizeUnits->Wrap( -1 );
	gbSizer1->Add( m_pinNameSizeUnits, wxGBPosition( 6, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	leftColumn->Add( gbSizer1, 0, wxEXPAND|wxALL, 5 );


	leftColumn->Add( 0, 15, 0, wxEXPAND, 5 );

	m_repeatLabel = new wxStaticText( this, wxID_ANY, _("Repeated Items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_repeatLabel->Wrap( -1 );
	leftColumn->Add( m_repeatLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	leftColumn->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	leftColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer2;
	gbSizer2 = new wxGridBagSizer( 5, 0 );
	gbSizer2->SetFlexibleDirection( wxBOTH );
	gbSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_pinPitchLabel = new wxStaticText( this, wxID_ANY, _("&Pitch of repeated pins:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinPitchLabel->Wrap( -1 );
	gbSizer2->Add( m_pinPitchLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_pinPitchCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer2->Add( m_pinPitchCtrl, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );

	m_pinPitchUnits = new wxStaticText( this, wxID_ANY, _("mils"), wxDefaultPosition, wxDefaultSize, 0 );
	m_pinPitchUnits->Wrap( -1 );
	gbSizer2->Add( m_pinPitchUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_labelIncrementLabel1 = new wxStaticText( this, wxID_ANY, _("Label increment:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelIncrementLabel1->Wrap( -1 );
	gbSizer2->Add( m_labelIncrementLabel1, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_spinRepeatLabel = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10, 10, 1 );
	gbSizer2->Add( m_spinRepeatLabel, wxGBPosition( 1, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT, 5 );


	leftColumn->Add( gbSizer2, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	p1mainSizer->Add( leftColumn, 1, wxEXPAND|wxRIGHT, 5 );


	p1mainSizer->Add( 25, 0, 0, 0, 5 );

	wxBoxSizer* rightColumn;
	rightColumn = new wxBoxSizer( wxVERTICAL );

	m_generalOption1 = new wxStaticText( this, wxID_ANY, _("General Editing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_generalOption1->Wrap( -1 );
	rightColumn->Add( m_generalOption1, 0, wxLEFT|wxRIGHT|wxTOP, 13 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	rightColumn->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	rightColumn->Add( 0, 5, 0, wxEXPAND, 5 );

	m_dragPinsWithEdges = new wxCheckBox( this, wxID_ANY, _("Keep pins attached when dragging edges"), wxDefaultPosition, wxDefaultSize, 0 );
	rightColumn->Add( m_dragPinsWithEdges, 0, wxALL, 5 );


	rightColumn->Add( 0, 15, 1, wxEXPAND, 5 );


	p1mainSizer->Add( rightColumn, 1, wxEXPAND|wxRIGHT, 5 );


	this->SetSizer( p1mainSizer );
	this->Layout();
	p1mainSizer->Fit( this );

	// Connect Events
	m_pinPitchCtrl->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_SYM_EDITING_OPTIONS_BASE::onKillFocusPinPitch ), NULL, this );
}

PANEL_SYM_EDITING_OPTIONS_BASE::~PANEL_SYM_EDITING_OPTIONS_BASE()
{
	// Disconnect Events
	m_pinPitchCtrl->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( PANEL_SYM_EDITING_OPTIONS_BASE::onKillFocusPinPitch ), NULL, this );

}
