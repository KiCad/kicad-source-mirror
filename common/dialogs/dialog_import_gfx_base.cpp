///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_import_gfx_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_GFX_BASE::DIALOG_IMPORT_GFX_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerFile;
	bSizerFile = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextFile = new wxStaticText( this, wxID_ANY, _("File:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFile->Wrap( -1 );
	bSizerFile->Add( m_staticTextFile, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_textCtrlFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
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

	m_importScaleCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, _("1.0"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_importScaleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizerImportSettings->Add( 0, 0, 0, 0, 5 );

	m_importWidthLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Imported width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importWidthLabel->Wrap( -1 );
	fgSizerImportSettings->Add( m_importWidthLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_importWidthCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_importWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_importWidthUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importWidthUnits->Wrap( -1 );
	fgSizerImportSettings->Add( m_importWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_importHeightLabel = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Imported height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importHeightLabel->Wrap( -1 );
	fgSizerImportSettings->Add( m_importHeightLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_importHeightCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerImportSettings->Add( m_importHeightCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_importHeightUnits = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importHeightUnits->Wrap( -1 );
	fgSizerImportSettings->Add( m_importHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizerImportSettings->Add( 0, 0, 1, wxEXPAND, 5 );

	m_aspectRatioCheckbox = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Lock height / width ratio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_aspectRatioCheckbox->SetValue(true);
	fgSizerImportSettings->Add( m_aspectRatioCheckbox, 0, wxALL, 5 );


	fgSizerImportSettings->Add( 0, 0, 1, wxEXPAND, 5 );


	bSizer7->Add( fgSizerImportSettings, 1, wxEXPAND|wxALL, 5 );


	sbSizer1->Add( bSizer7, 1, wxEXPAND, 5 );

	m_cbGroupItems = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Group imported items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbGroupItems->SetValue(true);
	m_cbGroupItems->SetToolTip( _("Add all imported items to a new group") );

	sbSizer1->Add( m_cbGroupItems, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


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

	m_imagePanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* bSizerImageOptions;
	bSizerImageOptions = new wxStaticBoxSizer( new wxStaticBox( m_imagePanel, wxID_ANY, _("Image Parameters") ), wxHORIZONTAL );

	wxBoxSizer* bSizerImageControls;
	bSizerImageControls = new wxBoxSizer( wxVERTICAL );

	m_thresholdLabel = new wxStaticText( bSizerImageOptions->GetStaticBox(), wxID_ANY, _("Black / white threshold:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thresholdLabel->Wrap( -1 );
	bSizerImageControls->Add( m_thresholdLabel, 0, wxALL, 5 );

	m_thresholdSlider = new wxSlider( bSizerImageOptions->GetStaticBox(), wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerImageControls->Add( m_thresholdSlider, 0, wxEXPAND, 5 );

	m_cbInvertColors = new wxCheckBox( bSizerImageOptions->GetStaticBox(), wxID_ANY, _("Invert Colors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerImageControls->Add( m_cbInvertColors, 0, wxALL, 5 );


	bSizerImageOptions->Add( bSizerImageControls, 1, wxEXPAND, 5 );

	m_previewBitmap = new wxStaticBitmap( bSizerImageOptions->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_previewBitmap->SetMinSize( wxSize( 160,120 ) );

	bSizerImageOptions->Add( m_previewBitmap, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_imagePanel->SetSizer( bSizerImageOptions );
	m_imagePanel->Layout();
	bSizerImageOptions->Fit( m_imagePanel );
	bSizerMain->Add( m_imagePanel, 0, wxEXPAND|wxLEFT|wxTOP, 5 );

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
	m_textCtrlFileName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onFilenameChanged ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onBrowseFiles ), NULL, this );
	m_rbInteractivePlacement->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onInteractivePlacement ), NULL, this );
	m_rbInteractivePlacement->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_rbAbsolutePlacement->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onAbsolutePlacement ), NULL, this );
	m_rbAbsolutePlacement->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_importScaleCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onScaleChanged ), NULL, this );
	m_importWidthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onWidthChanged ), NULL, this );
	m_importHeightCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onHeightChanged ), NULL, this );
	m_aspectRatioCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onLockAspectChanged ), NULL, this );
	m_thresholdSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_IMPORT_GFX_BASE::onThresholdChanged ), NULL, this );
	m_thresholdSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_IMPORT_GFX_BASE::onThresholdChanged ), NULL, this );
	m_cbInvertColors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onInvertChanged ), NULL, this );
}

DIALOG_IMPORT_GFX_BASE::~DIALOG_IMPORT_GFX_BASE()
{
	// Disconnect Events
	m_textCtrlFileName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onFilenameChanged ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onBrowseFiles ), NULL, this );
	m_rbInteractivePlacement->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onInteractivePlacement ), NULL, this );
	m_rbInteractivePlacement->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_rbAbsolutePlacement->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onAbsolutePlacement ), NULL, this );
	m_rbAbsolutePlacement->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GFX_BASE::originOptionOnUpdateUI ), NULL, this );
	m_importScaleCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onScaleChanged ), NULL, this );
	m_importWidthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onWidthChanged ), NULL, this );
	m_importHeightCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onHeightChanged ), NULL, this );
	m_aspectRatioCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onLockAspectChanged ), NULL, this );
	m_thresholdSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_IMPORT_GFX_BASE::onThresholdChanged ), NULL, this );
	m_thresholdSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_IMPORT_GFX_BASE::onThresholdChanged ), NULL, this );
	m_cbInvertColors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GFX_BASE::onInvertChanged ), NULL, this );

}
