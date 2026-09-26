///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/text_ctrl_eval.h"

#include "dialog_export_idf_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXPORT_IDF3_BASE::DIALOG_EXPORT_IDF3_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerIDFFile;
	bSizerIDFFile = new wxBoxSizer( wxVERTICAL );

	m_txtBrdFile = new wxStaticText( this, wxID_ANY, _("File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_txtBrdFile->Wrap( -1 );
	bSizerIDFFile->Add( m_txtBrdFile, 0, wxTOP|wxRIGHT|wxLEFT, 10 );

	m_filePickerIDF = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, _("Select an IDF export filename"), _("*.emn"), wxDefaultPosition, wxSize( 450,-1 ), wxFLP_OVERWRITE_PROMPT|wxFLP_SAVE|wxFLP_USE_TEXTCTRL );
	bSizerIDFFile->Add( m_filePickerIDF, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbsGeneral;
	sbsGeneral = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Board options") ), wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_outputUnitsLabel = new wxStaticText( sbsGeneral->GetStaticBox(), wxID_ANY, _("Output units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputUnitsLabel->Wrap( -1 );
	bSizer3->Add( m_outputUnitsLabel, 0, wxALL, 5 );

	wxString m_outputUnitsChoiceChoices[] = { _("Millimeters"), _("Mils") };
	int m_outputUnitsChoiceNChoices = sizeof( m_outputUnitsChoiceChoices ) / sizeof( wxString );
	m_outputUnitsChoice = new wxChoice( sbsGeneral->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_outputUnitsChoiceNChoices, m_outputUnitsChoiceChoices, 0 );
	m_outputUnitsChoice->SetSelection( 0 );
	bSizer3->Add( m_outputUnitsChoice, 0, wxALL, 5 );


	sbsGeneral->Add( bSizer3, 0, wxEXPAND, 5 );


	sbsGeneral->Add( 0, 20, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );

	m_staticText8 = new wxStaticText( sbsGeneral->GetStaticBox(), wxID_ANY, _("Coordinate Origin"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText8->Wrap( -1 );
	bSizer5->Add( m_staticText8, 0, wxALL, 5 );

	m_staticline1 = new wxStaticLine( sbsGeneral->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer5->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	m_rbOriginBoardCenter = new wxRadioButton( sbsGeneral->GetStaticBox(), wxID_ANY, _("Board center origin"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbOriginBoardCenter->SetValue( true );
	bSizer5->Add( m_rbOriginBoardCenter, 0, wxALL, 5 );

	m_rbOriginDrill = new wxRadioButton( sbsGeneral->GetStaticBox(), wxID_ANY, _("Drill/place file origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_rbOriginDrill, 0, wxALL, 5 );

	m_rbOriginGrid = new wxRadioButton( sbsGeneral->GetStaticBox(), wxID_ANY, _("Grid origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_rbOriginGrid, 0, wxALL, 5 );

	m_rbOriginUser = new wxRadioButton( sbsGeneral->GetStaticBox(), wxID_ANY, _("User defined origin"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_rbOriginUser, 0, wxALL, 5 );


	sbsGeneral->Add( bSizer5, 0, wxEXPAND, 5 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_xLabel = new wxStaticText( sbsGeneral->GetStaticBox(), wxID_ANY, _("X position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	gbSizer1->Add( m_xLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 23 );

	m_IDF_Xref = new TEXT_CTRL_EVAL( sbsGeneral->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_IDF_Xref->HasFlag( wxTE_MULTILINE ) )
	{
	m_IDF_Xref->SetMaxLength( 8 );
	}
	#else
	m_IDF_Xref->SetMaxLength( 8 );
	#endif
	gbSizer1->Add( m_IDF_Xref, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_xUnits = new wxStaticText( sbsGeneral->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnits->Wrap( -1 );
	gbSizer1->Add( m_xUnits, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_yLabel = new wxStaticText( sbsGeneral->GetStaticBox(), wxID_ANY, _("Y position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	gbSizer1->Add( m_yLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 23 );

	m_IDF_Yref = new TEXT_CTRL_EVAL( sbsGeneral->GetStaticBox(), wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_IDF_Yref->HasFlag( wxTE_MULTILINE ) )
	{
	m_IDF_Yref->SetMaxLength( 8 );
	}
	#else
	m_IDF_Yref->SetMaxLength( 8 );
	#endif
	gbSizer1->Add( m_IDF_Yref, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_yUnits = new wxStaticText( sbsGeneral->GetStaticBox(), wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	gbSizer1->Add( m_yUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	sbsGeneral->Add( gbSizer1, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bSizer6->Add( sbsGeneral, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbsComponents;
	sbsComponents = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Component options") ), wxVERTICAL );

	m_cbRemoveDNP = new wxCheckBox( sbsComponents->GetStaticBox(), wxID_ANY, _("Ignore 'Do not populate' components"), wxDefaultPosition, wxDefaultSize, 0 );
	sbsComponents->Add( m_cbRemoveDNP, 0, wxALL, 5 );

	m_cbRemoveUnspecified = new wxCheckBox( sbsComponents->GetStaticBox(), wxID_ANY, _("Ignore 'Unspecified' components"), wxDefaultPosition, wxDefaultSize, 0 );
	sbsComponents->Add( m_cbRemoveUnspecified, 0, wxALL, 5 );

	m_cbHeightFromModels = new wxCheckBox( sbsComponents->GetStaticBox(), wxID_ANY, _("Calculate component height from 3D models"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbHeightFromModels->SetValue(true);
	m_cbHeightFromModels->SetToolTip( _("When enabled, any footprint with an explicit height not specified will be exported with the height from its tallest visible 3D model.  When disabled, footprints without an explicit height will not be included in the IDF file.") );

	sbsComponents->Add( m_cbHeightFromModels, 0, wxALL, 5 );


	bSizer6->Add( sbsComponents, 1, wxALL|wxEXPAND, 5 );


	bSizer2->Add( bSizer6, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Log") ), wxVERTICAL );

	m_tcLog = new wxTextCtrl( sbSizer4->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,80 ), wxTE_MULTILINE|wxTE_READONLY );
	sbSizer4->Add( m_tcLog, 1, wxEXPAND, 5 );


	bSizer2->Add( sbSizer4, 0, wxALL|wxEXPAND, 5 );


	bSizerIDFFile->Add( bSizer2, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerIDFFile->Add( m_sdbSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	this->SetSizer( bSizerIDFFile );
	this->Layout();
	bSizerIDFFile->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_EXPORT_IDF3_BASE::~DIALOG_EXPORT_IDF3_BASE()
{
}
