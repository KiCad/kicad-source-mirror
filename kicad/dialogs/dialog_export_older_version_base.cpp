///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_older_version_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_OLDER_VERSION_BASE::DIALOG_EXPORT_OLDER_VERSION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerFields;
	fgSizerFields = new wxFlexGridSizer( 3, 2, 5, 5 );
	fgSizerFields->AddGrowableCol( 1 );
	fgSizerFields->SetFlexibleDirection( wxBOTH );
	fgSizerFields->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_targetLabel = new wxStaticText( this, wxID_ANY, _("Target version:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_targetLabel->Wrap( -1 );
	fgSizerFields->Add( m_targetLabel, 0, wxALL, 5 );

	wxArrayString m_targetChoiceChoices;
	m_targetChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_targetChoiceChoices, 0 );
	m_targetChoice->SetSelection( 0 );
	fgSizerFields->Add( m_targetChoice, 0, wxEXPAND, 5 );

	m_variantLabel = new wxStaticText( this, wxID_ANY, _("Variant:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_variantLabel->Wrap( -1 );
	fgSizerFields->Add( m_variantLabel, 0, wxALL, 5 );

	wxArrayString m_variantChoiceChoices;
	m_variantChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_variantChoiceChoices, 0 );
	m_variantChoice->SetSelection( 0 );
	fgSizerFields->Add( m_variantChoice, 0, wxEXPAND, 5 );

	m_destLabel = new wxStaticText( this, wxID_ANY, _("Destination folder:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_destLabel->Wrap( -1 );
	fgSizerFields->Add( m_destLabel, 0, wxALL, 5 );

	m_destPicker = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, _("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE );
	m_destPicker->SetMinSize( wxSize( 300,-1 ) );

	fgSizerFields->Add( m_destPicker, 0, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( fgSizerFields, 1, wxALL|wxEXPAND, 10 );

	m_cbDropInsteadOfApproximate = new wxCheckBox( this, wxID_ANY, _("Drop features instead of approximating them"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbDropInsteadOfApproximate->SetToolTip( _("Omit features that would otherwise be approximated. This may remove drawing or electrical data from the exported copy.") );

	bSizerMain->Add( m_cbDropInsteadOfApproximate, 0, wxALL|wxEXPAND, 10 );

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

DIALOG_EXPORT_OLDER_VERSION_BASE::~DIALOG_EXPORT_OLDER_VERSION_BASE()
{
}
