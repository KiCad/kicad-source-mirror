///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_target_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TARGET_PROPERTIES_BASE::DIALOG_TARGET_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_sizeLabel = new wxStaticText( this, wxID_ANY, _("Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeLabel->Wrap( -1 );
	fgSizer->Add( m_sizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_sizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_sizeCtrl, 0, wxALL|wxEXPAND, 5 );

	m_sizeUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sizeUnits->Wrap( -1 );
	fgSizer->Add( m_sizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgSizer->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer->Add( m_thicknessCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgSizer->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_staticTextShape = new wxStaticText( this, wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextShape->Wrap( -1 );
	fgSizer->Add( m_staticTextShape, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_TargetShapeChoices[] = { _("+"), _("X") };
	int m_TargetShapeNChoices = sizeof( m_TargetShapeChoices ) / sizeof( wxString );
	m_TargetShape = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_TargetShapeNChoices, m_TargetShapeChoices, 0 );
	m_TargetShape->SetSelection( 0 );
	fgSizer->Add( m_TargetShape, 0, wxALL|wxEXPAND, 5 );


	fgSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizerUpper->Add( fgSizer, 1, wxBOTTOM|wxEXPAND, 5 );


	bSizerMain->Add( bSizerUpper, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizerButts = new wxStdDialogButtonSizer();
	m_sdbSizerButtsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButts->AddButton( m_sdbSizerButtsOK );
	m_sdbSizerButtsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButts->AddButton( m_sdbSizerButtsCancel );
	m_sdbSizerButts->Realize();

	bSizerMain->Add( m_sdbSizerButts, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_TARGET_PROPERTIES_BASE::~DIALOG_TARGET_PROPERTIES_BASE()
{
}
