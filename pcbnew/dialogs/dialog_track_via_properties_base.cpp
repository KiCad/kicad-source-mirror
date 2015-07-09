///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "class_pcb_layer_box_selector.h"

#include "dialog_track_via_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TRACK_VIA_PROPERTIES_BASE::DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbTrackSizer;
	sbTrackSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgTrackLeftGridSizer;
	fgTrackLeftGridSizer = new wxFlexGridSizer( 4, 3, 0, 0 );
	fgTrackLeftGridSizer->SetFlexibleDirection( wxBOTH );
	fgTrackLeftGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_TrackStartXLabel = new wxStaticText( this, wxID_ANY, _("Start point X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartXLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartXLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackStartXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartXCtrl->SetMaxLength( 0 ); 
	fgTrackLeftGridSizer->Add( m_TrackStartXCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_TrackStartXUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartXUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartXUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackStartYLabel = new wxStaticText( this, wxID_ANY, _("Start point Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartYLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackStartYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYCtrl->SetMaxLength( 0 ); 
	fgTrackLeftGridSizer->Add( m_TrackStartYCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_TrackStartYUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartYUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackEndXLabel = new wxStaticText( this, wxID_ANY, _("End point X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndXLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndXLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackEndXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndXCtrl->SetMaxLength( 0 ); 
	fgTrackLeftGridSizer->Add( m_TrackEndXCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_TrackEndXUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndXUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndXUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackEndYLabel = new wxStaticText( this, wxID_ANY, _("End point Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndYLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_TrackEndYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYCtrl->SetMaxLength( 0 ); 
	fgTrackLeftGridSizer->Add( m_TrackEndYCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_TrackEndYUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndYUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbTrackSizer->Add( fgTrackLeftGridSizer, 1, wxEXPAND, 5 );
	
	m_trackStaticLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	sbTrackSizer->Add( m_trackStaticLine, 0, wxEXPAND | wxALL, 5 );
	
	wxFlexGridSizer* fgTrackRightSizer;
	fgTrackRightSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgTrackRightSizer->SetFlexibleDirection( wxBOTH );
	fgTrackRightSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_TrackWidthLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthLabel->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_TrackWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthCtrl->SetMaxLength( 0 ); 
	fgTrackRightSizer->Add( m_TrackWidthCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_TrackWidthUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthUnit->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackWidthUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgTrackRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_trackNetclass = new wxCheckBox( this, wxID_ANY, _("Use net class width"), wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackRightSizer->Add( m_trackNetclass, 0, wxALL, 5 );
	
	
	fgTrackRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_TrackLayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackLayerLabel->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackLayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_TrackLayerCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgTrackRightSizer->Add( m_TrackLayerCtrl, 0, wxALL, 5 );
	
	
	fgTrackRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	sbTrackSizer->Add( fgTrackRightSizer, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbTrackSizer, 0, wxEXPAND, 5 );
	
	m_mainSizerAccessor = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_mainSizerAccessor->Wrap( -1 );
	m_mainSizerAccessor->SetMaxSize( wxSize( 0,0 ) );
	
	bMainSizer->Add( m_mainSizerAccessor, 0, wxALL, 5 );
	
	wxStaticBoxSizer* sbViaSizer;
	sbViaSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgViaLeftSizer;
	fgViaLeftSizer = new wxFlexGridSizer( 2, 3, 0, 0 );
	fgViaLeftSizer->SetFlexibleDirection( wxBOTH );
	fgViaLeftSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_ViaXLabel = new wxStaticText( this, wxID_ANY, _("Position X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaXLabel->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaXLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ViaXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaXCtrl->SetMaxLength( 0 ); 
	fgViaLeftSizer->Add( m_ViaXCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_ViaXUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaXUnit->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaXUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ViaYLabel = new wxStaticText( this, wxID_ANY, _("Position Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYLabel->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaYLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ViaYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYCtrl->SetMaxLength( 0 ); 
	fgViaLeftSizer->Add( m_ViaYCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_ViaYUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYUnit->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaYUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	sbViaSizer->Add( fgViaLeftSizer, 1, wxEXPAND, 5 );
	
	m_viaStaticLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
	sbViaSizer->Add( m_viaStaticLine, 0, wxEXPAND | wxALL, 5 );
	
	wxFlexGridSizer* fgViaRightSizer;
	fgViaRightSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgViaRightSizer->SetFlexibleDirection( wxBOTH );
	fgViaRightSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_ViaDiameterLabel = new wxStaticText( this, wxID_ANY, _("Diameter"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterLabel->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaDiameterLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ViaDiameterCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterCtrl->SetMaxLength( 0 ); 
	fgViaRightSizer->Add( m_ViaDiameterCtrl, 0, wxEXPAND|wxALL, 5 );
	
	m_ViaDiameterUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterUnit->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaDiameterUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ViaDrillLabel = new wxStaticText( this, wxID_ANY, _("Drill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillLabel->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaDrillLabel, 0, wxALIGN_RIGHT|wxTOP|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_ViaDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillCtrl->SetMaxLength( 0 ); 
	fgViaRightSizer->Add( m_ViaDrillCtrl, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_ViaDrillUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillUnit->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaDrillUnit, 0, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_viaNetclass = new wxCheckBox( this, wxID_ANY, _("Use net class size"), wxDefaultPosition, wxDefaultSize, 0 );
	fgViaRightSizer->Add( m_viaNetclass, 0, wxALL, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	sbViaSizer->Add( fgViaRightSizer, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( sbViaSizer, 0, wxEXPAND, 5 );
	
	m_StdButtons = new wxStdDialogButtonSizer();
	m_StdButtonsOK = new wxButton( this, wxID_OK );
	m_StdButtons->AddButton( m_StdButtonsOK );
	m_StdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_StdButtons->AddButton( m_StdButtonsCancel );
	m_StdButtons->Realize();
	
	bMainSizer->Add( m_StdButtons, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onClose ) );
	m_trackNetclass->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTrackNetclassCheck ), NULL, this );
	m_viaNetclass->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaNetclassCheck ), NULL, this );
	m_StdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onCancelClick ), NULL, this );
	m_StdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onOkClick ), NULL, this );
}

DIALOG_TRACK_VIA_PROPERTIES_BASE::~DIALOG_TRACK_VIA_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onClose ) );
	m_trackNetclass->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTrackNetclassCheck ), NULL, this );
	m_viaNetclass->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaNetclassCheck ), NULL, this );
	m_StdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onCancelClick ), NULL, this );
	m_StdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onOkClick ), NULL, this );
	
}
