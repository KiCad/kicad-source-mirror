///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_image_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMAGE_PROPERTIES_BASE::DIALOG_IMAGE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );


	bMainSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerPos;
	fgSizerPos = new wxFlexGridSizer( 4, 3, 1, 0 );
	fgSizerPos->AddGrowableCol( 1 );
	fgSizerPos->AddGrowableRow( 0 );
	fgSizerPos->SetFlexibleDirection( wxBOTH );
	fgSizerPos->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_XPosLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_XPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ModPositionX = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModPositionX, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_XPosUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_XPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_YPosLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosLabel->Wrap( -1 );
	fgSizerPos->Add( m_YPosLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ModPositionY = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModPositionY, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP, 1 );

	m_YPosUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosUnit->Wrap( -1 );
	fgSizerPos->Add( m_YPosUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_RotationLabel = new wxStaticText( this, wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RotationLabel->Wrap( -1 );
	fgSizerPos->Add( m_RotationLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_ModRotation = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerPos->Add( m_ModRotation, 0, wxEXPAND, 5 );

	m_RotationUnit = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RotationUnit->Wrap( -1 );
	fgSizerPos->Add( m_RotationUnit, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_OpacityLabel = new wxStaticText( this, wxID_ANY, _("Opacity:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OpacityLabel->Wrap( -1 );
	fgSizerPos->Add( m_OpacityLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxLEFT|wxRIGHT, 5 );

	m_ModOpacity = new wxSpinCtrlDouble( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1, 1.000000, 0.1 );
	m_ModOpacity->SetDigits( 1 );
	fgSizerPos->Add( m_ModOpacity, 0, wxEXPAND, 5 );


	bMainSizer->Add( fgSizerPos, 0, wxEXPAND|wxALL, 8 );

	m_imageSizer = new wxBoxSizer( wxVERTICAL );

	m_imageSizer->SetMinSize( wxSize( 360,-1 ) );

	bMainSizer->Add( m_imageSizer, 1, wxEXPAND, 5 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	bMainSizer->Add( m_sdbSizerStdButtons, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_IMAGE_PROPERTIES_BASE::~DIALOG_IMAGE_PROPERTIES_BASE()
{
}
