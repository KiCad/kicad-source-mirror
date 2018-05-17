///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"
#include "widgets/text_ctrl_eval.h"
#include "widgets/widget_net_selector.h"

#include "dialog_track_via_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TRACK_VIA_PROPERTIES_BASE::DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	m_sbCommonSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Common:") ), wxHORIZONTAL );
	
	m_staticText24 = new wxStaticText( m_sbCommonSizer->GetStaticBox(), wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	m_sbCommonSizer->Add( m_staticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_NetComboBox = new WIDGET_NET_SELECTOR( m_sbCommonSizer->GetStaticBox(), wxID_ANY, _("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_DROPDOWN|wxCB_READONLY ); 
	m_sbCommonSizer->Add( m_NetComboBox, 1, wxALL|wxEXPAND, 5 );
	
	m_lockedCbox = new wxCheckBox( m_sbCommonSizer->GetStaticBox(), wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, wxCHK_3STATE );
	m_sbCommonSizer->Add( m_lockedCbox, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	
	m_MainSizer->Add( m_sbCommonSizer, 0, wxEXPAND|wxALL, 5 );
	
	m_sbTrackSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks:") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgTrackLeftGridSizer;
	fgTrackLeftGridSizer = new wxFlexGridSizer( 4, 3, 5, 5 );
	fgTrackLeftGridSizer->SetFlexibleDirection( wxBOTH );
	fgTrackLeftGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_TrackStartXLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Start point X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartXLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartXLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_TrackStartXCtrl = new TEXT_CTRL_EVAL( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackLeftGridSizer->Add( m_TrackStartXCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_TrackStartXUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartXUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartXUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_TrackStartYLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Start point Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartYLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_TrackStartYCtrl = new TEXT_CTRL_EVAL( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackLeftGridSizer->Add( m_TrackStartYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_TrackStartYUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackStartYUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackStartYUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_TrackEndXLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("End point X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndXLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndXLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_TrackEndXCtrl = new TEXT_CTRL_EVAL( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackLeftGridSizer->Add( m_TrackEndXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_TrackEndXUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndXUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndXUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_TrackEndYLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("End point Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYLabel->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndYLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_TrackEndYCtrl = new TEXT_CTRL_EVAL( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackLeftGridSizer->Add( m_TrackEndYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_TrackEndYUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackEndYUnit->Wrap( -1 );
	fgTrackLeftGridSizer->Add( m_TrackEndYUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_sbTrackSizer->Add( fgTrackLeftGridSizer, 1, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgTrackRightSizer;
	fgTrackRightSizer = new wxFlexGridSizer( 4, 3, 5, 5 );
	fgTrackRightSizer->SetFlexibleDirection( wxBOTH );
	fgTrackRightSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_TrackWidthLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthLabel->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackWidthLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_TrackWidthCtrl = new wxComboBox( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER ); 
	fgTrackRightSizer->Add( m_TrackWidthCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_TrackWidthUnit = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthUnit->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackWidthUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	fgTrackRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_trackNetclass = new wxCheckBox( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Use net class width"), wxDefaultPosition, wxDefaultSize, 0 );
	fgTrackRightSizer->Add( m_trackNetclass, 0, wxBOTTOM|wxTOP, 5 );
	
	
	fgTrackRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_TrackLayerLabel = new wxStaticText( m_sbTrackSizer->GetStaticBox(), wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackLayerLabel->Wrap( -1 );
	fgTrackRightSizer->Add( m_TrackLayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_TrackLayerCtrl = new PCB_LAYER_BOX_SELECTOR( m_sbTrackSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgTrackRightSizer->Add( m_TrackLayerCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	fgTrackRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_sbTrackSizer->Add( fgTrackRightSizer, 1, wxEXPAND, 5 );
	
	
	m_MainSizer->Add( m_sbTrackSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_sbViaSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias:") ), wxHORIZONTAL );
	
	wxFlexGridSizer* fgViaLeftSizer;
	fgViaLeftSizer = new wxFlexGridSizer( 4, 3, 5, 5 );
	fgViaLeftSizer->SetFlexibleDirection( wxBOTH );
	fgViaLeftSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_ViaXLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaXLabel->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaXLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_ViaXCtrl = new TEXT_CTRL_EVAL( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgViaLeftSizer->Add( m_ViaXCtrl, 0, wxEXPAND, 5 );
	
	m_ViaXUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaXUnit->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaXUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_ViaYLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYLabel->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaYLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_ViaYCtrl = new TEXT_CTRL_EVAL( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgViaLeftSizer->Add( m_ViaYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_ViaYUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaYUnit->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaYUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_ViaDiameterLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Diameter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterLabel->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaDiameterLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_ViaDiameterCtrl = new TEXT_CTRL_EVAL( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgViaLeftSizer->Add( m_ViaDiameterCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_ViaDiameterUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDiameterUnit->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaDiameterUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_ViaDrillLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Drill:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillLabel->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaDrillLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_ViaDrillCtrl = new TEXT_CTRL_EVAL( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgViaLeftSizer->Add( m_ViaDrillCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_ViaDrillUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDrillUnit->Wrap( -1 );
	fgViaLeftSizer->Add( m_ViaDrillUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_sbViaSizer->Add( fgViaLeftSizer, 1, wxEXPAND|wxALL, 5 );
	
	wxFlexGridSizer* fgViaRightSizer;
	fgViaRightSizer = new wxFlexGridSizer( 5, 3, 5, 5 );
	fgViaRightSizer->SetFlexibleDirection( wxBOTH );
	fgViaRightSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_DesignRuleVias = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Design rule vias:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DesignRuleVias->Wrap( -1 );
	fgViaRightSizer->Add( m_DesignRuleVias, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	wxArrayString m_DesignRuleViasCtrlChoices;
	m_DesignRuleViasCtrl = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_DesignRuleViasCtrlChoices, 0 );
	m_DesignRuleViasCtrl->SetSelection( 0 );
	fgViaRightSizer->Add( m_DesignRuleViasCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	m_DesignRuleViasUnit = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DesignRuleViasUnit->Wrap( -1 );
	fgViaRightSizer->Add( m_DesignRuleViasUnit, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_ViaTypeLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Via type:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaTypeLabel->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaTypeLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxString m_ViaTypeChoiceChoices[] = { _("Through"), _("Micro"), _("Blind/buried"), wxEmptyString };
	int m_ViaTypeChoiceNChoices = sizeof( m_ViaTypeChoiceChoices ) / sizeof( wxString );
	m_ViaTypeChoice = new wxChoice( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ViaTypeChoiceNChoices, m_ViaTypeChoiceChoices, 0 );
	m_ViaTypeChoice->SetSelection( 0 );
	m_ViaTypeChoice->Enable( false );
	
	fgViaRightSizer->Add( m_ViaTypeChoice, 0, wxEXPAND, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_ViaStartLayerLabel = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Start layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaStartLayerLabel->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaStartLayerLabel, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_ViaStartLayer = new PCB_LAYER_BOX_SELECTOR( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgViaRightSizer->Add( m_ViaStartLayer, 0, wxEXPAND, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_ViaEndLayerLabel1 = new wxStaticText( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("End layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaEndLayerLabel1->Wrap( -1 );
	fgViaRightSizer->Add( m_ViaEndLayerLabel1, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );
	
	m_ViaEndLayer = new PCB_LAYER_BOX_SELECTOR( m_sbViaSizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgViaRightSizer->Add( m_ViaEndLayer, 0, wxEXPAND, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_viaNetclass = new wxCheckBox( m_sbViaSizer->GetStaticBox(), wxID_ANY, _("Use net class size"), wxDefaultPosition, wxDefaultSize, 0 );
	fgViaRightSizer->Add( m_viaNetclass, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxTOP, 5 );
	
	
	fgViaRightSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	m_sbViaSizer->Add( fgViaRightSizer, 1, wxEXPAND|wxALL, 5 );
	
	
	m_MainSizer->Add( m_sbViaSizer, 0, wxALL|wxEXPAND, 5 );
	
	m_StdButtons = new wxStdDialogButtonSizer();
	m_StdButtonsOK = new wxButton( this, wxID_OK );
	m_StdButtons->AddButton( m_StdButtonsOK );
	m_StdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_StdButtons->AddButton( m_StdButtonsCancel );
	m_StdButtons->Realize();
	
	m_MainSizer->Add( m_StdButtons, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onClose ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::OnInitDlg ) );
	m_TrackWidthCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onOkClick ), NULL, this );
	m_trackNetclass->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTrackNetclassCheck ), NULL, this );
	m_viaNetclass->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaNetclassCheck ), NULL, this );
	m_StdButtonsCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onCancelClick ), NULL, this );
	m_StdButtonsOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onOkClick ), NULL, this );
}

DIALOG_TRACK_VIA_PROPERTIES_BASE::~DIALOG_TRACK_VIA_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onClose ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::OnInitDlg ) );
	m_TrackWidthCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onOkClick ), NULL, this );
	m_trackNetclass->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onTrackNetclassCheck ), NULL, this );
	m_viaNetclass->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onViaNetclassCheck ), NULL, this );
	m_StdButtonsCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onCancelClick ), NULL, this );
	m_StdButtonsOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES_BASE::onOkClick ), NULL, this );
	
}
