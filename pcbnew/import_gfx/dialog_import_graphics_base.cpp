///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/std_bitmap_button.h"

#include "dialog_import_graphics_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_GRAPHICS_BASE::DIALOG_IMPORT_GRAPHICS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
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

	bSizerFile->Add( m_textCtrlFileName, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bSizerFile->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerFile, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 3, 5, 3 );
	fgSizer3->AddGrowableCol( 1 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_importScaleLabel = new wxStaticText( this, wxID_ANY, _("Import scale:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importScaleLabel->Wrap( -1 );
	fgSizer3->Add( m_importScaleLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_importScaleCtrl = new wxTextCtrl( this, wxID_ANY, _("1.0"), wxDefaultPosition, wxSize( 120,-1 ), 0 );
	fgSizer3->Add( m_importScaleCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_importWidthLabel = new wxStaticText( this, wxID_ANY, _("Imported width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importWidthLabel->Wrap( -1 );
	fgSizer3->Add( m_importWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_importWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_importWidthCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgSizer3->Add( m_importWidthCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_importWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importWidthUnits->Wrap( -1 );
	fgSizer3->Add( m_importWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_importHeightLabel = new wxStaticText( this, wxID_ANY, _("Imported height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importHeightLabel->Wrap( -1 );
	fgSizer3->Add( m_importHeightLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_importHeightCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_importHeightCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgSizer3->Add( m_importHeightCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_importHeightUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_importHeightUnits->Wrap( -1 );
	fgSizer3->Add( m_importHeightUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_aspectRatioCheckbox = new wxCheckBox( this, wxID_ANY, _("Lock height / width ratio"), wxDefaultPosition, wxDefaultSize, 0 );
	m_aspectRatioCheckbox->SetValue(true);
	fgSizer3->Add( m_aspectRatioCheckbox, 0, wxALL, 5 );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_lineWidthLabel = new wxStaticText( this, wxID_ANY, _("DXF default line width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthLabel->Wrap( -1 );
	m_lineWidthLabel->SetToolTip( _("Used when the DXF items in file have no line thickness set") );

	fgSizer3->Add( m_lineWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_lineWidthCtrl = new wxTextCtrl( this, wxID_ANY, _("0.2"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer3->Add( m_lineWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_lineWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lineWidthUnits->Wrap( -1 );
	fgSizer3->Add( m_lineWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_dxfUnitsLabel = new wxStaticText( this, wxID_ANY, _("DXF default units:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_dxfUnitsLabel->Wrap( -1 );
	m_dxfUnitsLabel->SetToolTip( _("Used when the DXF file has no unit set") );

	fgSizer3->Add( m_dxfUnitsLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_dxfUnitsChoiceChoices;
	m_dxfUnitsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_dxfUnitsChoiceChoices, 0 );
	m_dxfUnitsChoice->SetSelection( 0 );
	fgSizer3->Add( m_dxfUnitsChoice, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bSizerMain->Add( fgSizer3, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_imagePanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerImageOptions;
	bSizerImageOptions = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerImageControls;
	bSizerImageControls = new wxBoxSizer( wxVERTICAL );

	m_thresholdLabel = new wxStaticText( m_imagePanel, wxID_ANY, _("Black / white threshold:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thresholdLabel->Wrap( -1 );
	bSizerImageControls->Add( m_thresholdLabel, 0, wxALL, 5 );

	m_thresholdSlider = new wxSlider( m_imagePanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_LABELS );
	bSizerImageControls->Add( m_thresholdSlider, 0, wxEXPAND|wxLEFT, 5 );

	m_cbInvertColors = new wxCheckBox( m_imagePanel, wxID_ANY, _("Invert Colors"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerImageControls->Add( m_cbInvertColors, 0, wxEXPAND|wxLEFT, 5 );


	bSizerImageOptions->Add( bSizerImageControls, 1, wxEXPAND|wxLEFT, 5 );

	m_previewBitmap = new wxStaticBitmap( m_imagePanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_previewBitmap->SetMinSize( wxSize( 160,120 ) );

	bSizerImageOptions->Add( m_previewBitmap, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	m_imagePanel->SetSizer( bSizerImageOptions );
	m_imagePanel->Layout();
	bSizerImageOptions->Fit( m_imagePanel );
	bSizerMain->Add( m_imagePanel, 0, wxEXPAND, 5 );


	bSizerMain->Add( 0, 5, 0, wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline2, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizerPlaceAt;
	bSizerPlaceAt = new wxBoxSizer( wxHORIZONTAL );

	m_placeAtCheckbox = new wxCheckBox( this, wxID_ANY, _("Place at:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_placeAtCheckbox->SetToolTip( _("If not checked: use interactive placement.") );

	bSizerPlaceAt->Add( m_placeAtCheckbox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_xLabel = new wxStaticText( this, wxID_ANY, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_xLabel->Wrap( -1 );
	bSizerPlaceAt->Add( m_xLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_xCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_xCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_xCtrl->SetMaxLength( 10 );
	}
	#else
	m_xCtrl->SetMaxLength( 10 );
	#endif
	m_xCtrl->SetToolTip( _("DXF origin on PCB Grid, X Coordinate") );

	bSizerPlaceAt->Add( m_xCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_yLabel = new wxStaticText( this, wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yLabel->Wrap( -1 );
	bSizerPlaceAt->Add( m_yLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_yCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	#ifdef __WXGTK__
	if ( !m_yCtrl->HasFlag( wxTE_MULTILINE ) )
	{
	m_yCtrl->SetMaxLength( 10 );
	}
	#else
	m_yCtrl->SetMaxLength( 10 );
	#endif
	m_yCtrl->SetToolTip( _("DXF origin on PCB Grid, Y Coordinate") );

	bSizerPlaceAt->Add( m_yCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_yUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_yUnits->Wrap( -1 );
	bSizerPlaceAt->Add( m_yUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_originLabel = new wxStaticText( this, wxID_ANY, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_originLabel->Wrap( -1 );
	bSizerPlaceAt->Add( m_originLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 10 );

	wxString m_originCtrlChoices[] = { _("Page origin"), _("Drill/place file origin"), _("Grid origin"), _("User defined origin") };
	int m_originCtrlNChoices = sizeof( m_originCtrlChoices ) / sizeof( wxString );
	m_originCtrl = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_originCtrlNChoices, m_originCtrlChoices, 0 );
	m_originCtrl->SetSelection( 0 );
	bSizerPlaceAt->Add( m_originCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerPlaceAt, 1, wxEXPAND|wxLEFT, 5 );

	wxBoxSizer* bSizerTargetLayer;
	bSizerTargetLayer = new wxBoxSizer( wxHORIZONTAL );

	m_radioBtnCurrentLayer = new wxRadioButton( this, wxID_ANY, _("Current layer"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_radioBtnCurrentLayer->SetValue( true );
	bSizerTargetLayer->Add( m_radioBtnCurrentLayer, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_radioBtnMapLayers = new wxRadioButton( this, wxID_ANY, _("Map layers"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTargetLayer->Add( m_radioBtnMapLayers, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_radioBtnSingleLayer = new wxRadioButton( this, wxID_ANY, _("Single layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTargetLayer->Add( m_radioBtnSingleLayer, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_SelLayerBox = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizerTargetLayer->Add( m_SelLayerBox, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerTargetLayer, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerGroupOpt;
	bSizerGroupOpt = new wxBoxSizer( wxVERTICAL );

	m_cbGroupItems = new wxCheckBox( this, wxID_ANY, _("Group imported items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbGroupItems->SetValue(true);
	m_cbGroupItems->SetToolTip( _("Add all imported items to a new group") );

	bSizerGroupOpt->Add( m_cbGroupItems, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerMain->Add( bSizerGroupOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline3, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_rbFixDiscontinuities = new wxCheckBox( this, wxID_ANY, _("Fix discontinuities"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbFixDiscontinuities->SetValue(true);
	m_rbFixDiscontinuities->SetToolTip( _("Trim/extend open shapes or add segments to make vertices of shapes coincide") );

	bSizer11->Add( m_rbFixDiscontinuities, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_toleranceLabel = new wxStaticText( this, wxID_ANY, _("Tolerance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toleranceLabel->Wrap( -1 );
	bSizer11->Add( m_toleranceLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 30 );

	m_toleranceCtrl = new wxTextCtrl( this, wxID_ANY, _("1"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	bSizer11->Add( m_toleranceCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_toleranceUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_toleranceUnits->Wrap( -1 );
	bSizer11->Add( m_toleranceUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizerMain->Add( bSizer11, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	bSizerMain->Add( 0, 3, 1, wxEXPAND, 5 );

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
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onUpdateUI ) );
	m_textCtrlFileName->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onFilenameChanged ), NULL, this );
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onBrowseFiles ), NULL, this );
	m_importScaleCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onScaleChanged ), NULL, this );
	m_importWidthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onWidthChanged ), NULL, this );
	m_importHeightCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onHeightChanged ), NULL, this );
	m_aspectRatioCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onLockAspectChanged ), NULL, this );
	m_thresholdSlider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onThresholdChanged ), NULL, this );
	m_thresholdSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onThresholdChanged ), NULL, this );
	m_cbInvertColors->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onInvertChanged ), NULL, this );
}

DIALOG_IMPORT_GRAPHICS_BASE::~DIALOG_IMPORT_GRAPHICS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onUpdateUI ) );
	m_textCtrlFileName->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onFilenameChanged ), NULL, this );
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onBrowseFiles ), NULL, this );
	m_importScaleCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onScaleChanged ), NULL, this );
	m_importWidthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onWidthChanged ), NULL, this );
	m_importHeightCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onHeightChanged ), NULL, this );
	m_aspectRatioCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onLockAspectChanged ), NULL, this );
	m_thresholdSlider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onThresholdChanged ), NULL, this );
	m_thresholdSlider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onThresholdChanged ), NULL, this );
	m_cbInvertColors->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_GRAPHICS_BASE::onInvertChanged ), NULL, this );

}
