///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_increment_annotations_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_INCREMENT_ANNOTATIONS_BASE::DIALOG_INCREMENT_ANNOTATIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer31;
	fgSizer31 = new wxFlexGridSizer( 0, 2, 6, 6 );
	fgSizer31->AddGrowableCol( 1 );
	fgSizer31->SetFlexibleDirection( wxBOTH );
	fgSizer31->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_firstRefDesLabel = new wxStaticText( this, wxID_ANY, _("Start reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_firstRefDesLabel->Wrap( -1 );
	m_firstRefDesLabel->SetToolTip( _("The symbol name in library and also the default\nsymbol value when loaded in the schematic.") );

	fgSizer31->Add( m_firstRefDesLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_FirstRefDes = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	fgSizer31->Add( m_FirstRefDes, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_incrementLabel = new wxStaticText( this, wxID_ANY, _("Increment by:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_incrementLabel->Wrap( -1 );
	fgSizer31->Add( m_incrementLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_Increment = new wxSpinCtrl( this, wxID_ANY, wxT("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64, 0 );
	fgSizer31->Add( m_Increment, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bSizerTop->Add( fgSizer31, 1, wxALL|wxEXPAND, 5 );


	bSizerTop->Add( 0, 10, 0, wxEXPAND, 5 );

	m_CurrentSheet = new wxRadioButton( this, wxID_ANY, _("Current sheet only"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_CurrentSheet->SetValue( true );
	bSizerTop->Add( m_CurrentSheet, 0, wxALL, 5 );

	m_AllSheets = new wxRadioButton( this, wxID_ANY, _("All sheets"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTop->Add( m_AllSheets, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizerMain->Add( bSizerTop, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_INCREMENT_ANNOTATIONS_BASE::~DIALOG_INCREMENT_ANNOTATIONS_BASE()
{
}
