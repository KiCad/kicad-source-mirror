///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_enum_pads_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ENUM_PADS_BASE::DIALOG_ENUM_PADS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_lblInfo = new wxStaticText( this, wxID_ANY, _("Pad names are restricted to 4 characters (including number)."), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblInfo->Wrap( -1 );
	bMainSizer->Add( m_lblInfo, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	bMainSizer->Add( 0, 0, 0, wxTOP|wxBOTTOM, 5 );

	wxFlexGridSizer* fgSizer;
	fgSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer->AddGrowableCol( 1 );
	fgSizer->SetFlexibleDirection( wxBOTH );
	fgSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblPadPrefix = new wxStaticText( this, wxID_ANY, _("Pad name prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPadPrefix->Wrap( -1 );
	fgSizer->Add( m_lblPadPrefix, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_padPrefix = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_padPrefix->HasFlag( wxTE_MULTILINE ) )
	{
	m_padPrefix->SetMaxLength( 4 );
	}
	#else
	m_padPrefix->SetMaxLength( 4 );
	#endif
	fgSizer->Add( m_padPrefix, 0, wxALL|wxEXPAND, 5 );

	m_lblPadStartNum = new wxStaticText( this, wxID_ANY, _("First pad number:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPadStartNum->Wrap( -1 );
	fgSizer->Add( m_lblPadStartNum, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_padStartNum = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999, 1 );
	fgSizer->Add( m_padStartNum, 0, wxALL|wxEXPAND, 5 );

	m_lblPadNumStep = new wxStaticText( this, wxID_ANY, _("Numbering step:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblPadNumStep->Wrap( -1 );
	fgSizer->Add( m_lblPadNumStep, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_padNumStep = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999, 1 );
	fgSizer->Add( m_padNumStep, 0, wxALL|wxEXPAND, 5 );


	bMainSizer->Add( fgSizer, 1, wxEXPAND|wxALL, 5 );

	m_stdButtons = new wxStdDialogButtonSizer();
	m_stdButtonsOK = new wxButton( this, wxID_OK );
	m_stdButtons->AddButton( m_stdButtonsOK );
	m_stdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_stdButtons->AddButton( m_stdButtonsCancel );
	m_stdButtons->Realize();

	bMainSizer->Add( m_stdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_ENUM_PADS_BASE::~DIALOG_ENUM_PADS_BASE()
{
}
