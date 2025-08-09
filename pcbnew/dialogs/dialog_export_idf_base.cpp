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

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 2, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_cbSetBoardReferencePoint = new wxCheckBox( this, wxID_ANY, _("Set board reference point:"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
	gbSizer1->Add( m_cbSetBoardReferencePoint, wxGBPosition( 0, 0 ), wxGBSpan( 1, 3 ), 0, 5 );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("X position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	gbSizer1->Add( m_xLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 23 );

	m_IDF_Xref = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_IDF_Xref->HasFlag( wxTE_MULTILINE ) )
	{
	m_IDF_Xref->SetMaxLength( 8 );
	}
	#else
	m_IDF_Xref->SetMaxLength( 8 );
	#endif
	gbSizer1->Add( m_IDF_Xref, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_xUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnits->Wrap( -1 );
	gbSizer1->Add( m_xUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_yLabel = new wxStaticText( this, wxID_ANY, _("Y position:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	gbSizer1->Add( m_yLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 23 );

	m_IDF_Yref = new TEXT_CTRL_EVAL( this, wxID_ANY, _("0"), wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_IDF_Yref->HasFlag( wxTE_MULTILINE ) )
	{
	m_IDF_Yref->SetMaxLength( 8 );
	}
	#else
	m_IDF_Yref->SetMaxLength( 8 );
	#endif
	gbSizer1->Add( m_IDF_Yref, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_yUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	gbSizer1->Add( m_yUnits, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_outputUnitsLabel = new wxStaticText( this, wxID_ANY, _("Output units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_outputUnitsLabel->Wrap( -1 );
	gbSizer1->Add( m_outputUnitsLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 8 );

	wxString m_outputUnitsChoiceChoices[] = { _("Millimeters"), _("Mils") };
	int m_outputUnitsChoiceNChoices = sizeof( m_outputUnitsChoiceChoices ) / sizeof( wxString );
	m_outputUnitsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_outputUnitsChoiceNChoices, m_outputUnitsChoiceChoices, 0 );
	m_outputUnitsChoice->SetSelection( 0 );
	gbSizer1->Add( m_outputUnitsChoice, wxGBPosition( 3, 1 ), wxGBSpan( 1, 2 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_cbRemoveDNP = new wxCheckBox( this, wxID_ANY, _("Ignore 'Do not populate' components"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbRemoveDNP, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxBOTTOM, 5 );

	m_cbRemoveUnspecified = new wxCheckBox( this, wxID_ANY, _("Ignore 'Unspecified' components"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbRemoveUnspecified, wxGBPosition( 5, 0 ), wxGBSpan( 1, 3 ), 0, 5 );


	bSizerIDFFile->Add( gbSizer1, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

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
