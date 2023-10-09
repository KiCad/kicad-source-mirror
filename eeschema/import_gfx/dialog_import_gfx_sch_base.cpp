///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_import_gfx_sch_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_GFX_SCH_BASE::DIALOG_IMPORT_GFX_SCH_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerFile;
	bSizerFile = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFile->Wrap( -1 );
	m_staticTextFile->SetToolTip( _("Only vectors will be imported.  Bitmaps and fonts will be ignored.") );

	bSizerFile->Add( m_staticTextFile, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textCtrlFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlFileName->SetToolTip( _("Only vectors will be imported.  Bitmaps and fonts will be ignored.") );
	m_textCtrlFileName->SetMinSize( wxSize( 300,-1 ) );

	bSizerFile->Add( m_textCtrlFileName, 1, wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerFile->Add( m_browseButton, 0, wxTOP|wxBOTTOM, 5 );


	bSizerMain->Add( bSizerFile, 0, wxALL|wxEXPAND, 10 );

	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Placement") ), wxVERTICAL );

	wxBoxSizer* bSizerOptions;
	bSizerOptions = new wxBoxSizer( wxVERTICAL );

	m_rbInteractivePlacement = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_ANY, _("Interactive placement"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbInteractivePlacement->SetValue( true );
	bSizerOptions->Add( m_rbInteractivePlacement, 0, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizerUserPos;
	bSizerUserPos = new wxBoxSizer( wxHORIZONTAL );

	m_rbAbsolutePlacement = new wxRadioButton( sbSizer2->GetStaticBox(), wxID_ANY, _("At"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerUserPos->Add( m_rbAbsolutePlacement, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizerPosSettings;
	bSizerPosSettings = new wxBoxSizer( wxHORIZONTAL );

	m_xLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	bSizerPosSettings->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_xCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_xCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_xCtrl->SetMaxLength( 10 );
	}
	#else
	m_xCtrl->SetMaxLength( 10 );
	#endif
	m_xCtrl->SetToolTip( _("DXF origin on PCB Grid, X Coordinate") );

	bSizerPosSettings->Add( m_xCtrl, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_xUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xUnits->Wrap( -1 );
	bSizerPosSettings->Add( m_xUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );

	m_yLabel = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	bSizerPosSettings->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_yCtrl = new wxTextCtrl( sbSizer2->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_yCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_yCtrl->SetMaxLength( 10 );
	}
	#else
	m_yCtrl->SetMaxLength( 10 );
	#endif
	m_yCtrl->SetToolTip( _("DXF origin on PCB Grid, Y Coordinate") );

	bSizerPosSettings->Add( m_yCtrl, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_yUnits = new wxStaticText( sbSizer2->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	bSizerPosSettings->Add( m_yUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerUserPos->Add( bSizerPosSettings, 1, wxBOTTOM|wxEXPAND|wxRIGHT|wxTOP, 5 );


	bSizerOptions->Add( bSizerUserPos, 0, wxEXPAND, 5 );


	sbSizer2->Add( bSizerOptions, 0, wxEXPAND|wxTOP|wxLEFT, 5 );


	bSizerMain->Add( sbSizer2, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Import Parameters") ), wxVERTICAL );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizerImportSettings;
	fgSizerImportSettings = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgSizerImportSettings->AddGrowableCol( 1 );
	fgSizerImportSettings->SetFlexibleDirection( wxBOTH );
	fgSizerImportSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_importScaleLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Import scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importScaleLabel->Wrap( -1 );
	fgSizerImportSettings->Add( m_importScaleLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_importScaleCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_importScaleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizerImportSettings->Add( 0, 0, 0, 0, 5 );


	bSizer7->Add( fgSizerImportSettings, 1, wxEXPAND|wxALL, 5 );


	sbSizer1->Add( bSizer7, 1, wxEXPAND, 5 );


	bSizerMain->Add( sbSizer1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("DXF Parameters") ), wxVERTICAL );

	wxBoxSizer* bSizer81;
	bSizer81 = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgDxfImportSettings;
	fgDxfImportSettings = new wxFlexGridSizer( 0, 3, 5, 5 );
	fgDxfImportSettings->AddGrowableCol( 1 );
	fgDxfImportSettings->SetFlexibleDirection( wxBOTH );
	fgDxfImportSettings->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lineWidthLabel = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	fgDxfImportSettings->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_lineWidthCtrl = new wxTextCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgDxfImportSettings->Add( m_lineWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lineWidthUnits = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgDxfImportSettings->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );

	m_staticTextLineWidth1 = new wxStaticText( sbSizer3->GetStaticBox(), wxID_ANY, _("Default units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLineWidth1->Wrap( -1 );
	fgDxfImportSettings->Add( m_staticTextLineWidth1, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxArrayString m_choiceDxfUnitsChoices;
	m_choiceDxfUnits = new wxChoice( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceDxfUnitsChoices, 0 );
	m_choiceDxfUnits->SetSelection( 0 );
	fgDxfImportSettings->Add( m_choiceDxfUnits, 0, wxEXPAND, 5 );


	fgDxfImportSettings->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer81->Add( fgDxfImportSettings, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbSizer3->Add( bSizer81, 1, wxEXPAND, 5 );


	bSizerMain->Add( sbSizer3, 1, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bSizerMain->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::onBrowseFiles ), NULL, this );
	m_rbInteractivePlacement->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::onInteractivePlacement ), NULL, this );
	m_rbInteractivePlacement->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::originOptionOnUpdateUI ), NULL, this );
	m_rbAbsolutePlacement->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::onAbsolutePlacement ), NULL, this );
	m_rbAbsolutePlacement->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::originOptionOnUpdateUI ), NULL, this );
}

DIALOG_IMPORT_GFX_SCH_BASE::~DIALOG_IMPORT_GFX_SCH_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::onBrowseFiles ), NULL, this );
	m_rbInteractivePlacement->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::onInteractivePlacement ), NULL, this );
	m_rbInteractivePlacement->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::originOptionOnUpdateUI ), NULL, this );
	m_rbAbsolutePlacement->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::onAbsolutePlacement ), NULL, this );
	m_rbAbsolutePlacement->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_SCH_BASE::originOptionOnUpdateUI ), NULL, this );

}
