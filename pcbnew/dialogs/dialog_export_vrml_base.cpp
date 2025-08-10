///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_export_vrml_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_VRML_BASE::DIALOG_EXPORT_VRML_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxVERTICAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bUpperSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_filePicker = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Save VRML Board File"), _("*.wrl"), wxDefaultPosition, wxDefaultSize, wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	m_filePicker->SetMinSize( wxSize( 450,-1 ) );

	bUpperSizer->Add( m_filePicker, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Footprint 3D model path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bUpperSizer->Add( m_staticText3, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_SubdirNameCtrl = new wxTextCtrl( this, wxID_ANY, _("shapes3D"), wxDefaultPosition, wxDefaultSize, 0 );
	bUpperSizer->Add( m_SubdirNameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bSizer1->Add( bUpperSizer, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerOptions;
	bSizerOptions = new wxBoxSizer( wxVERTICAL );

	m_cbUserDefinedOrigin = new wxCheckBox( this, wxID_ANY, _("User defined origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUserDefinedOrigin->SetValue(true);
	bSizerOptions->Add( m_cbUserDefinedOrigin, 0, wxALL, 5 );

	wxFlexGridSizer* fgSizerOptions;
	fgSizerOptions = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizerOptions->AddGrowableCol( 1 );
	fgSizerOptions->SetFlexibleDirection( wxBOTH );
	fgSizerOptions->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	fgSizerOptions->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_VRML_Xref = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_VRML_Xref->HasFlag( wxTE_MULTILINE ) )
	{
	m_VRML_Xref->SetMaxLength( 8 );
	}
	#else
	m_VRML_Xref->SetMaxLength( 8 );
	#endif
	fgSizerOptions->Add( m_VRML_Xref, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_xUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnits->Wrap( -1 );
	fgSizerOptions->Add( m_xUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_yLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	fgSizerOptions->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_VRML_Yref = new wxTextCtrl( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_VRML_Yref->HasFlag( wxTE_MULTILINE ) )
	{
	m_VRML_Yref->SetMaxLength( 8 );
	}
	#else
	m_VRML_Yref->SetMaxLength( 8 );
	#endif
	fgSizerOptions->Add( m_VRML_Yref, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_yUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	fgSizerOptions->Add( m_yUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerOptions->Add( fgSizerOptions, 0, wxEXPAND|wxRIGHT|wxLEFT, 20 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_unitsLabel = new wxStaticText( this, wxID_ANY, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_unitsLabel->Wrap( -1 );
	bSizer7->Add( m_unitsLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_unitsChoiceChoices[] = { _("mm"), _("meter"), _("0.1 inch"), _("inch") };
	int m_unitsChoiceNChoices = sizeof( m_unitsChoiceChoices ) / sizeof( wxString );
	m_unitsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_unitsChoiceNChoices, m_unitsChoiceChoices, 0 );
	m_unitsChoice->SetSelection( 1 );
	bSizer7->Add( m_unitsChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerOptions->Add( bSizer7, 0, wxEXPAND|wxTOP, 5 );


	bSizer1->Add( bSizerOptions, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_cbRemoveDNP = new wxCheckBox( this, wxID_ANY, _("Ignore 'Do not populate' components"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbRemoveDNP, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_cbRemoveUnspecified = new wxCheckBox( this, wxID_ANY, _("Ignore 'Unspecified' components"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cbRemoveUnspecified, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_cbCopyFiles = new wxCheckBox( this, wxID_ANY, _("Copy 3D model files to 3D model path"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbCopyFiles->SetToolTip( _("If checked: copy 3D models to the destination folder\nIf not checked: Embed 3D models in the VRML board file") );

	bSizer4->Add( m_cbCopyFiles, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_cbUseRelativePaths = new wxCheckBox( this, wxID_ANY, _("Use relative paths to model files in board VRML file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbUseRelativePaths->SetToolTip( _("Use paths for model files in board VRML file relative to the VRML file") );

	bSizer4->Add( m_cbUseRelativePaths, 0, wxALL, 5 );


	bLowerSizer->Add( bSizer4, 2, wxEXPAND, 5 );


	bSizer1->Add( bLowerSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizer1->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	// Connect Events
	m_cbUseRelativePaths->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_VRML_BASE::OnUpdateUseRelativePath ), NULL, this );
}

DIALOG_EXPORT_VRML_BASE::~DIALOG_EXPORT_VRML_BASE()
{
	// Disconnect Events
	m_cbUseRelativePaths->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_EXPORT_VRML_BASE::OnUpdateUseRelativePath ), NULL, this );

}
