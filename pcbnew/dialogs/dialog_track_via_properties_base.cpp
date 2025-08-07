///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_track_via_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TRACK_VIA_PROPERTIES_BASE::DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	m_sbCommonSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Common") ), wxVERTICAL );

	wxBoxSizer* bSizerNetWidgets;
	bSizerNetWidgets = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bNetWidgetsLeftCol;
	bNetWidgetsLeftCol = new wxBoxSizer( wxHORIZONTAL );

	m_netSelectorLabel = new wxStaticText( m_sbCommonSizer->GetStaticBox(), wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_netSelectorLabel->Wrap( -1 );
	bNetWidgetsLeftCol->Add( m_netSelectorLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_netSelector = new NET_SELECTOR( m_sbCommonSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	bNetWidgetsLeftCol->Add( m_netSelector, 1, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerNetWidgets->Add( bNetWidgetsLeftCol, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 10 );

	wxBoxSizer* bNetWidgetsRightCol;
	bNetWidgetsRightCol = new wxBoxSizer( wxHORIZONTAL );

	m_viaNotFree = new wxCheckBox( m_sbCommonSizer->GetStaticBox(), wxID_ANY, _("Automatically update via nets"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	m_viaNotFree->SetToolTip( _("Automatically change the net of this via when the pads or zones it touches are changed") );

	bNetWidgetsRightCol->Add( m_viaNotFree, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerNetWidgets->Add( bNetWidgetsRightCol, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	m_sbCommonSizer->Add( bSizerNetWidgets, 5, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticline1 = new wxStaticLine( m_sbCommonSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_sbCommonSizer->Add( m_staticline1, 0, wxEXPAND|wxBOTTOM, 2 );


	m_sbCommonSizer->Add( 0, 5, 0, wxEXPAND, 5 );

	m_lockedCbox = new wxCheckBox( m_sbCommonSizer->GetStaticBox(), wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	m_sbCommonSizer->Add( m_lockedCbox, 0, wxTOP|wxBOTTOM|wxLEFT, 4 );


	m_MainSizer->Add( m_sbCommonSizer, 0, wxEXPAND|wxALL, 10 );

	m_sbTrackSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks") ), wxHORIZONTAL );

	wxBoxSizer* bSizerTracksLeftCol;
	bSizerTracksLeftCol = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgTrackEnds;
	fgTrackEnds = new wxFlexGridSizer( 7, 5, 3, 3 );
	fgTrackEnds->AddGrowableCol( 1 );
	fgTrackEnds->AddGrowableCol( 3 );
	fgTrackEnds->SetFlexibleDirection( wxBOTH );
	fgTrackEnds->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_TrackStartXLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Start X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartXLabel->Wrap( -1 );
	fgTrackEnds->Add( m_TrackStartXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_TrackStartXCtrl = new wxTextCtrl( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackEnds->Add( m_TrackStartXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_TrackStartYLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYLabel->Wrap( -1 );
	fgTrackEnds->Add( m_TrackStartYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 8 );

	m_TrackStartYCtrl = new wxTextCtrl( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackEnds->Add( m_TrackStartYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_TrackStartYUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYUnit->Wrap( -1 );
	fgTrackEnds->Add( m_TrackStartYUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_TrackEndXLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("End X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndXLabel->Wrap( -1 );
	fgTrackEnds->Add( m_TrackEndXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_TrackEndXCtrl = new wxTextCtrl( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackEnds->Add( m_TrackEndXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_TrackEndYLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYLabel->Wrap( -1 );
	fgTrackEnds->Add( m_TrackEndYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 8 );

	m_TrackEndYCtrl = new wxTextCtrl( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackEnds->Add( m_TrackEndYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_TrackEndYUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYUnit->Wrap( -1 );
	fgTrackEnds->Add( m_TrackEndYUnit, 0, wxALIGN_CENTER_VERTICAL, 8 );


	bSizerTracksLeftCol->Add( fgTrackEnds, 0, wxEXPAND|wxBOTTOM, 8 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 3 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,0 ) );

	m_predefinedTrackWidthsLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Pre-defined sizes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_predefinedTrackWidthsLabel->Wrap( -1 );
	gbSizer1->Add( m_predefinedTrackWidthsLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	wxArrayString m_predefinedTrackWidthsCtrlChoices;
	m_predefinedTrackWidthsCtrl = new wxChoice( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_predefinedTrackWidthsCtrlChoices, 0 );
	m_predefinedTrackWidthsCtrl->SetSelection( 0 );
	gbSizer1->Add( m_predefinedTrackWidthsCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );

	m_predefinedTrackWidthsUnits = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_predefinedTrackWidthsUnits->Wrap( -1 );
	gbSizer1->Add( m_predefinedTrackWidthsUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_TrackWidthLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthLabel->Wrap( -1 );
	gbSizer1->Add( m_TrackWidthLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_TrackWidthCtrl = new wxTextCtrl( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_TrackWidthCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_TrackWidthUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthUnit->Wrap( -1 );
	gbSizer1->Add( m_TrackWidthUnit, wxGBPosition( 2, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bSizerTracksLeftCol->Add( gbSizer1, 1, wxEXPAND|wxTOP, 5 );


	m_sbTrackSizer->Add( bSizerTracksLeftCol, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_sbTrackSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxBoxSizer* bSizerTrackRightControl;
	bSizerTrackRightControl = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgTrackRightSizer;
	fgTrackRightSizer = new wxFlexGridSizer( 1, 3, 3, 5 );
	fgTrackRightSizer->AddGrowableCol( 2 );
	fgTrackRightSizer->SetFlexibleDirection( wxBOTH );
	fgTrackRightSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_TrackLayerLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackLayerLabel->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackLayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	fgTrackRightSizer->Add( 25, 0, 0, wxEXPAND, 5 );

	m_TrackLayerCtrl = new PCB_LAYER_BOX_SELECTOR( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgTrackRightSizer->Add( m_TrackLayerCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	bSizerTrackRightControl->Add( fgTrackRightSizer, 0, wxEXPAND|wxRIGHT, 3 );

	wxBoxSizer* bSizerTechLayers;
	bSizerTechLayers = new wxBoxSizer( wxVERTICAL );


	bSizerTechLayers->Add( 0, 15, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer26;
	bSizer26 = new wxBoxSizer( wxHORIZONTAL );

	m_techLayersLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Technical Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_techLayersLabel->Wrap( -1 );
	bSizer26->Add( m_techLayersLabel, 0, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline3 = new wxStaticLine( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer26->Add( m_staticline3, 1, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 3 );


	bSizerTechLayers->Add( bSizer26, 0, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerTrackMaskMargin;
	fgSizerTrackMaskMargin = new wxFlexGridSizer( 0, 4, 3, 5 );
	fgSizerTrackMaskMargin->AddGrowableCol( 2 );
	fgSizerTrackMaskMargin->SetFlexibleDirection( wxBOTH );
	fgSizerTrackMaskMargin->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_trackHasSolderMask = new wxCheckBox( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Solder mask"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	fgSizerTrackMaskMargin->Add( m_trackHasSolderMask, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_trackMaskMarginLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Expansion:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackMaskMarginLabel->Wrap( -1 );
	fgSizerTrackMaskMargin->Add( m_trackMaskMarginLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_trackMaskMarginCtrl = new wxTextCtrl( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_trackMaskMarginCtrl->SetToolTip( _("This is the local clearance between the track and the solder mask opening.\nLeave blank to use the value defined in the Board Setup.") );

	fgSizerTrackMaskMargin->Add( m_trackMaskMarginCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_trackMaskMarginUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_trackMaskMarginUnit->Wrap( -1 );
	fgSizerTrackMaskMargin->Add( m_trackMaskMarginUnit, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerTechLayers->Add( fgSizerTrackMaskMargin, 1, wxEXPAND|wxTOP, 2 );


	bSizerTrackRightControl->Add( bSizerTechLayers, 1, wxEXPAND, 3 );


	m_sbTrackSizer->Add( bSizerTrackRightControl, 1, wxEXPAND, 5 );


	m_MainSizer->Add( m_sbTrackSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	m_sbViaSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias") ), wxVERTICAL );

	wxBoxSizer* bSizerViaCols;
	bSizerViaCols = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerViasLeftCol;
	bSizerViasLeftCol = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgViaPos;
	fgViaPos = new wxFlexGridSizer( 6, 5, 4, 2 );
	fgViaPos->AddGrowableCol( 1 );
	fgViaPos->AddGrowableCol( 3 );
	fgViaPos->SetFlexibleDirection( wxBOTH );
	fgViaPos->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ViaXLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaXLabel->Wrap( -1 );
	fgViaPos->Add( m_ViaXLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_ViaXCtrl = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgViaPos->Add( m_ViaXCtrl, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 3 );

	m_ViaYLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYLabel->Wrap( -1 );
	fgViaPos->Add( m_ViaYLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 8 );

	m_ViaYCtrl = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgViaPos->Add( m_ViaYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 3 );

	m_ViaYUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYUnit->Wrap( -1 );
	fgViaPos->Add( m_ViaYUnit, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizerViasLeftCol->Add( fgViaPos, 0, wxEXPAND|wxBOTTOM, 8 );

	wxGridBagSizer* gbSizer3;
	gbSizer3 = new wxGridBagSizer( 3, 3 );
	gbSizer3->SetFlexibleDirection( wxBOTH );
	gbSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer3->SetEmptyCellSize( wxSize( -1,0 ) );

	m_predefinedViaSizesLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Pre-defined sizes:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_predefinedViaSizesLabel->Wrap( -1 );
	gbSizer3->Add( m_predefinedViaSizesLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	wxArrayString m_predefinedViaSizesCtrlChoices;
	m_predefinedViaSizesCtrl = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_predefinedViaSizesCtrlChoices, 0 );
	m_predefinedViaSizesCtrl->SetSelection( 0 );
	gbSizer3->Add( m_predefinedViaSizesCtrl, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 3 );

	m_predefinedViaSizesUnits = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_predefinedViaSizesUnits->Wrap( -1 );
	gbSizer3->Add( m_predefinedViaSizesUnits, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_sbPadstackSettings = new wxBoxSizer( wxHORIZONTAL );

	m_stPadstackMode = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Padstack mode:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stPadstackMode->Wrap( -1 );
	m_sbPadstackSettings->Add( m_stPadstackMode, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_cbPadstackModeChoices[] = { _("Normal"), _("Front/Inner/Back"), _("Custom") };
	int m_cbPadstackModeNChoices = sizeof( m_cbPadstackModeChoices ) / sizeof( wxString );
	m_cbPadstackMode = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbPadstackModeNChoices, m_cbPadstackModeChoices, 0 );
	m_cbPadstackMode->SetSelection( 0 );
	m_sbPadstackSettings->Add( m_cbPadstackMode, 0, wxALL, 5 );

	m_stEditLayer = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Edit layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stEditLayer->Wrap( -1 );
	m_sbPadstackSettings->Add( m_stEditLayer, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	wxArrayString m_cbEditLayerChoices;
	m_cbEditLayer = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbEditLayerChoices, 0 );
	m_cbEditLayer->SetSelection( 0 );
	m_sbPadstackSettings->Add( m_cbEditLayer, 0, wxALL, 5 );


	gbSizer3->Add( m_sbPadstackSettings, wxGBPosition( 2, 0 ), wxGBSpan( 1, 3 ), wxEXPAND, 0 );

	m_ViaDiameterLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Via diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterLabel->Wrap( -1 );
	gbSizer3->Add( m_ViaDiameterLabel, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxLEFT|wxALIGN_CENTER_VERTICAL, 1 );

	m_ViaDiameterCtrl = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_ViaDiameterCtrl, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_ViaDiameterUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterUnit->Wrap( -1 );
	gbSizer3->Add( m_ViaDiameterUnit, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaDrillLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Via hole:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillLabel->Wrap( -1 );
	gbSizer3->Add( m_ViaDrillLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT, 1 );

	m_ViaDrillCtrl = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer3->Add( m_ViaDrillCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_ViaDrillUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillUnit->Wrap( -1 );
	gbSizer3->Add( m_ViaDrillUnit, wxGBPosition( 4, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer3->AddGrowableCol( 1 );

	bSizerViasLeftCol->Add( gbSizer3, 1, wxEXPAND|wxTOP, 5 );


	bSizerViaCols->Add( bSizerViasLeftCol, 1, wxEXPAND, 5 );


	bSizerViaCols->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxBoxSizer* viaRightColumn;
	viaRightColumn = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer4;
	gbSizer4 = new wxGridBagSizer( 0, 0 );
	gbSizer4->SetFlexibleDirection( wxBOTH );
	gbSizer4->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_ViaTypeLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Via type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaTypeLabel->Wrap( -1 );
	gbSizer4->Add( m_ViaTypeLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_ViaTypeChoiceChoices[] = { _("Through"), _("Micro"), _("Blind/buried") };
	int m_ViaTypeChoiceNChoices = sizeof( m_ViaTypeChoiceChoices ) / sizeof( wxString );
	m_ViaTypeChoice = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ViaTypeChoiceNChoices, m_ViaTypeChoiceChoices, 0 );
	m_ViaTypeChoice->SetSelection( 0 );
	m_ViaTypeChoice->Enable( false );

	gbSizer4->Add( m_ViaTypeChoice, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaStartLayerLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Start layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaStartLayerLabel->Wrap( -1 );
	gbSizer4->Add( m_ViaStartLayerLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaStartLayer = new PCB_LAYER_BOX_SELECTOR( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer4->Add( m_ViaStartLayer, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_ViaEndLayerLabel1 = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("End layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaEndLayerLabel1->Wrap( -1 );
	gbSizer4->Add( m_ViaEndLayerLabel1, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_ViaEndLayer = new PCB_LAYER_BOX_SELECTOR( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer4->Add( m_ViaEndLayer, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_annularRingsLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Annular rings:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_annularRingsLabel->Wrap( -1 );
	gbSizer4->Add( m_annularRingsLabel, wxGBPosition( 4, 0 ), wxGBSpan( 1, 1 ), wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_annularRingsCtrlChoices[] = { _("All copper layers"), _("Start, end, and connected layers"), _("Connected layers only"), _("Start and end layers only") };
	int m_annularRingsCtrlNChoices = sizeof( m_annularRingsCtrlChoices ) / sizeof( wxString );
	m_annularRingsCtrl = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_annularRingsCtrlNChoices, m_annularRingsCtrlChoices, 0 );
	m_annularRingsCtrl->SetSelection( 0 );
	gbSizer4->Add( m_annularRingsCtrl, wxGBPosition( 4, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 4 );

	m_protectionPresetsLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Protection features:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_protectionPresetsLabel->Wrap( -1 );
	gbSizer4->Add( m_protectionPresetsLabel, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxArrayString m_protectionFeaturesChoices;
	m_protectionFeatures = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_protectionFeaturesChoices, 0 );
	m_protectionFeatures->SetSelection( 0 );
	m_protectionFeatures->SetToolTip( _("Select which protection feature according to IPC-4761 the via should have.") );

	gbSizer4->Add( m_protectionFeatures, wxGBPosition( 5, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 4 );


	gbSizer4->AddGrowableCol( 1 );

	viaRightColumn->Add( gbSizer4, 1, wxEXPAND, 5 );


	bSizerViaCols->Add( viaRightColumn, 1, wxEXPAND, 5 );


	m_sbViaSizer->Add( bSizerViaCols, 0, wxALL|wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_sbViaSizer->Add( m_staticline2, 0, wxEXPAND|wxBOTTOM, 8 );

	m_legacyTeardropsWarning = new wxBoxSizer( wxHORIZONTAL );

	m_legacyTeardropsIcon = new wxStaticBitmap( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_legacyTeardropsWarning->Add( m_legacyTeardropsIcon, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	wxBoxSizer* bSizer42;
	bSizer42 = new wxBoxSizer( wxVERTICAL );

	m_staticText85 = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Board contains legacy teardrops."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText85->Wrap( -1 );
	bSizer42->Add( m_staticText85, 0, wxRIGHT|wxLEFT, 5 );

	m_staticText851 = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Use Edit > Edit Teardrops to apply automatic teardrops."), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText851->Wrap( -1 );
	bSizer42->Add( m_staticText851, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_legacyTeardropsWarning->Add( bSizer42, 1, wxALIGN_CENTER_VERTICAL, 5 );


	m_sbViaSizer->Add( m_legacyTeardropsWarning, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );

	wxBoxSizer* bSizerTeardrops;
	bSizerTeardrops = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerCols11;
	bSizerCols11 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol11;
	bSizerLeftCol11 = new wxBoxSizer( wxVERTICAL );


	bSizerLeftCol11->Add( 0, 2, 0, wxEXPAND, 5 );

	m_cbTeardrops = new wxCheckBox( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Add teardrops on via's track connections"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	bSizerLeftCol11->Add( m_cbTeardrops, 0, wxBOTTOM|wxRIGHT, 3 );

	m_cbTeardropsUseNextTrack = new wxCheckBox( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Allow teardrops to span two track segments"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	m_cbTeardropsUseNextTrack->SetToolTip( _("Allows a teardrop to extend over the first 2 connected track segments if the first track segment is too short to accommodate the best length.") );

	bSizerLeftCol11->Add( m_cbTeardropsUseNextTrack, 0, wxBOTTOM|wxRIGHT, 5 );


	bSizerCols11->Add( bSizerLeftCol11, 1, wxEXPAND|wxTOP|wxLEFT, 1 );


	bSizerCols11->Add( 20, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerRightCol11;
	bSizerRightCol11 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer39;
	bSizer39 = new wxBoxSizer( wxHORIZONTAL );

	m_stHDRatio = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Maximum track width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatio->Wrap( -1 );
	m_stHDRatio->SetToolTip( _("Max pad/via size to track width ratio to create a teardrop.\n100 always creates a teardrop.") );

	bSizer39->Add( m_stHDRatio, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_tcHDRatio = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_tcHDRatio->SetToolTip( _("Tracks which are similar in size to the via do not need teardrops.") );

	bSizer39->Add( m_tcHDRatio, 0, wxRIGHT|wxLEFT, 5 );

	m_stHDRatioUnits = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("%"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stHDRatioUnits->Wrap( -1 );
	bSizer39->Add( m_stHDRatioUnits, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerRightCol11->Add( bSizer39, 0, wxEXPAND, 3 );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );

	m_minTrackWidthHint = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("(as a percentage of pad/via/track size)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_minTrackWidthHint->Wrap( -1 );
	bSizer17->Add( m_minTrackWidthHint, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );


	bSizerRightCol11->Add( bSizer17, 0, wxEXPAND|wxLEFT, 5 );


	bSizerCols11->Add( bSizerRightCol11, 1, wxEXPAND|wxLEFT, 10 );


	bSizerTeardrops->Add( bSizerCols11, 0, wxEXPAND|wxTOP, 3 );


	bSizerTeardrops->Add( 0, 5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerShapeColumns;
	bSizerShapeColumns = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerLeftCol;
	bSizerLeftCol = new wxBoxSizer( wxVERTICAL );

	m_bitmapTeardrop = new wxStaticBitmap( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLeftCol->Add( m_bitmapTeardrop, 1, wxALIGN_CENTER_HORIZONTAL, 5 );


	bSizerShapeColumns->Add( bSizerLeftCol, 1, wxEXPAND|wxRIGHT, 10 );


	bSizerShapeColumns->Add( 20, 0, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizerRightCol;
	fgSizerRightCol = new wxFlexGridSizer( 0, 3, 3, 5 );
	fgSizerRightCol->AddGrowableCol( 1 );
	fgSizerRightCol->SetFlexibleDirection( wxBOTH );
	fgSizerRightCol->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_stLenPercentLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Best length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentLabel->Wrap( -1 );
	fgSizerRightCol->Add( m_stLenPercentLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_tcLenPercent = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcLenPercent, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer131;
	bSizer131 = new wxBoxSizer( wxHORIZONTAL );

	m_stLenPercentUnits = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentUnits->Wrap( -1 );
	bSizer131->Add( m_stLenPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stLenPercentHint = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentHint->Wrap( -1 );
	m_stLenPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer131->Add( m_stLenPercentHint, 0, wxALIGN_BOTTOM, 1 );

	m_stLenPercentSuffix = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stLenPercentSuffix->Wrap( -1 );
	bSizer131->Add( m_stLenPercentSuffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRightCol->Add( bSizer131, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLen = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Maximum length (L):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLen->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxLen, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_tcTdMaxLen = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcTdMaxLen, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxLenUnits = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxLenUnits->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxLenUnits, 0, wxALIGN_CENTER_VERTICAL, 3 );


	fgSizerRightCol->Add( 0, 3, 1, wxEXPAND, 5 );


	fgSizerRightCol->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizerRightCol->Add( 0, 0, 1, wxEXPAND, 5 );

	m_stWidthPercentLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Best width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentLabel->Wrap( -1 );
	fgSizerRightCol->Add( m_stWidthPercentLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_tcWidthPercent = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcWidthPercent, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer1311;
	bSizer1311 = new wxBoxSizer( wxHORIZONTAL );

	m_stWidthPercentUnits = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("%("), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentUnits->Wrap( -1 );
	bSizer1311->Add( m_stWidthPercentUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stWidthPercentHint = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("d"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentHint->Wrap( -1 );
	m_stWidthPercentHint->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer1311->Add( m_stWidthPercentHint, 0, wxALIGN_BOTTOM, 1 );

	m_stWidthPercentSuffix = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _(" )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stWidthPercentSuffix->Wrap( -1 );
	bSizer1311->Add( m_stWidthPercentSuffix, 0, wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerRightCol->Add( bSizer1311, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidthLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Maximum width (W):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthLabel->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_tcMaxWidth = new wxTextCtrl( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerRightCol->Add( m_tcMaxWidth, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_stMaxWidthUnits = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stMaxWidthUnits->Wrap( -1 );
	fgSizerRightCol->Add( m_stMaxWidthUnits, 0, wxALIGN_CENTER_VERTICAL, 3 );


	bSizer20->Add( fgSizerRightCol, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );

	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxHORIZONTAL );

	m_curvedEdges = new wxCheckBox( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Curved edges"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	bSizer44->Add( m_curvedEdges, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer44->Add( 45, 0, 0, 0, 5 );


	bSizer20->Add( bSizer44, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	bSizerShapeColumns->Add( bSizer20, 1, wxEXPAND, 5 );


	bSizerTeardrops->Add( bSizerShapeColumns, 1, wxEXPAND|wxBOTTOM, 3 );


	m_sbViaSizer->Add( bSizerTeardrops, 1, wxEXPAND, 5 );


	m_MainSizer->Add( m_sbViaSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_StdButtons = new wxStdDialogButtonSizer();
	m_StdButtonsOK = new wxButton( this, wxID_OK );
	m_StdButtons->AddButton( m_StdButtonsOK );
	m_StdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_StdButtons->AddButton( m_StdButtonsCancel );
	m_StdButtons->Realize();

	m_MainSizer->Add( m_StdButtons, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_viaNotFree->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaNotFreeClicked ), NULL, this );
	m_predefinedTrackWidthsCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onWidthSelect ), NULL, this );
	m_TrackWidthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onWidthEdit ), NULL, this );
	m_trackHasSolderMask->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTrackEdit ), NULL, this );
	m_predefinedViaSizesCtrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaSelect ), NULL, this );
	m_cbPadstackMode->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onPadstackModeChanged ), NULL, this );
	m_cbEditLayer->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onEditLayerChanged ), NULL, this );
	m_ViaDiameterCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaDrillCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaTypeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaStartLayer->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaEndLayer->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_cbTeardrops->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_cbTeardropsUseNextTrack->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHDRatio->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcHDRatio->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHDRatioUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_minTrackWidthHint->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_bitmapTeardrop->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcLenPercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentHint->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentSuffix->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLen->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcTdMaxLen->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLenUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcWidthPercent->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentHint->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentSuffix->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxWidthLabel->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcMaxWidth->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxWidthUnits->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_curvedEdges->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
}

DIALOG_TRACK_VIA_PROPERTIES_BASE::~DIALOG_TRACK_VIA_PROPERTIES_BASE()
{
	// Disconnect Events
	m_viaNotFree->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaNotFreeClicked ), NULL, this );
	m_predefinedTrackWidthsCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onWidthSelect ), NULL, this );
	m_TrackWidthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onWidthEdit ), NULL, this );
	m_trackHasSolderMask->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTrackEdit ), NULL, this );
	m_predefinedViaSizesCtrl->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaSelect ), NULL, this );
	m_cbPadstackMode->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onPadstackModeChanged ), NULL, this );
	m_cbEditLayer->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onEditLayerChanged ), NULL, this );
	m_ViaDiameterCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaDrillCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaTypeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaStartLayer->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_ViaEndLayer->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaEdit ), NULL, this );
	m_cbTeardrops->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_cbTeardropsUseNextTrack->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHDRatio->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcHDRatio->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stHDRatioUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_minTrackWidthHint->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_bitmapTeardrop->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcLenPercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentHint->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stLenPercentSuffix->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLen->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcTdMaxLen->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxLenUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcWidthPercent->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentHint->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stWidthPercentSuffix->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxWidthLabel->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_tcMaxWidth->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_stMaxWidthUnits->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );
	m_curvedEdges->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTeardropsUpdateUi ), NULL, this );

}
