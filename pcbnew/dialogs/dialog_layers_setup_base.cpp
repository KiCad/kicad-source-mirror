///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_layers_setup_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LAYERS_SETUP_BASE::DIALOG_LAYERS_SETUP_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 550,600 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bChoicesSizer;
	bChoicesSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bPresetsSizer;
	bPresetsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextGrouping = new wxStaticText( this, wxID_ANY, _("Preset Layer Groupings"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextGrouping->Wrap( -1 );
	bPresetsSizer->Add( m_staticTextGrouping, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_PresetsChoiceChoices[] = { _("Custom"), _("Two layers, parts on Front only"), _("Two layers, parts on Back only"), _("Two layers, parts on Front and Back"), _("Four layers, parts on Front only"), _("Four layers, parts on Front and Back"), _("All layers on") };
	int m_PresetsChoiceNChoices = sizeof( m_PresetsChoiceChoices ) / sizeof( wxString );
	m_PresetsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PresetsChoiceNChoices, m_PresetsChoiceChoices, 0 );
	m_PresetsChoice->SetSelection( 0 );
	bPresetsSizer->Add( m_PresetsChoice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bChoicesSizer->Add( bPresetsSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bCopperLayersSizer;
	bCopperLayersSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextCopperLayers = new wxStaticText( this, wxID_ANY, _("Copper Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCopperLayers->Wrap( -1 );
	bCopperLayersSizer->Add( m_staticTextCopperLayers, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_CopperLayersChoiceChoices[] = { _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16"), _("18"), _("20"), _("22"), _("24"), _("26"), _("28"), _("30"), _("32") };
	int m_CopperLayersChoiceNChoices = sizeof( m_CopperLayersChoiceChoices ) / sizeof( wxString );
	m_CopperLayersChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_CopperLayersChoiceNChoices, m_CopperLayersChoiceChoices, 0 );
	m_CopperLayersChoice->SetSelection( 0 );
	bCopperLayersSizer->Add( m_CopperLayersChoice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	
	bChoicesSizer->Add( bCopperLayersSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bBrdThicknessSizer;
	bBrdThicknessSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextBrdThickness = new wxStaticText( this, wxID_ANY, _("Board Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdThickness->Wrap( -1 );
	bBrdThicknessSizer->Add( m_staticTextBrdThickness, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxFlexGridSizer* fgSizerBrdThickness;
	fgSizerBrdThickness = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerBrdThickness->AddGrowableCol( 0 );
	fgSizerBrdThickness->SetFlexibleDirection( wxBOTH );
	fgSizerBrdThickness->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_textCtrlBrdThickness = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerBrdThickness->Add( m_textCtrlBrdThickness, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticTextBrdThicknessUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextBrdThicknessUnit->Wrap( -1 );
	fgSizerBrdThickness->Add( m_staticTextBrdThicknessUnit, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );
	
	
	bBrdThicknessSizer->Add( fgSizerBrdThickness, 1, wxEXPAND, 5 );
	
	
	bChoicesSizer->Add( bBrdThicknessSizer, 1, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bChoicesSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* b_layersListSizer;
	b_layersListSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLayers = new wxStaticText( this, wxID_ANY, _("Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayers->Wrap( -1 );
	b_layersListSizer->Add( m_staticTextLayers, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bCaptionsSizer;
	bCaptionsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_TitlePanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxRAISED_BORDER|wxTAB_TRAVERSAL );
	m_TitlePanel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );
	m_TitlePanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVECAPTION ) );
	m_TitlePanel->SetMinSize( wxSize( -1,15 ) );
	
	bCaptionsSizer->Add( m_TitlePanel, 1, wxEXPAND, 5 );
	
	
	b_layersListSizer->Add( bCaptionsSizer, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_LayersListPanel = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxRAISED_BORDER|wxTAB_TRAVERSAL|wxVSCROLL );
	m_LayersListPanel->SetScrollRate( 0, 5 );
	m_LayerListFlexGridSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	m_LayerListFlexGridSizer->AddGrowableCol( 0 );
	m_LayerListFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_LayerListFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_CrtYdFrontName = new wxStaticText( m_LayersListPanel, ID_CRTYDFRONTNAME, _("CrtYd_Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CrtYdFrontName, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_CrtYdFrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_CrtYdFrontPanel->SetBackgroundColour( wxColour( 255, 233, 236 ) );
	
	wxBoxSizer* bSizer611;
	bSizer611 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer611->Add( 0, 0, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 15 );
	
	m_CrtYdFrontCheckBox = new wxCheckBox( m_CrtYdFrontPanel, ID_CRTYDFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_CrtYdFrontCheckBox->SetToolTip( _("If you want a courtyard layer for the front side of the board") );
	
	bSizer611->Add( m_CrtYdFrontCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );
	
	
	bSizer611->Add( 0, 0, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 15 );
	
	
	m_CrtYdFrontPanel->SetSizer( bSizer611 );
	m_CrtYdFrontPanel->Layout();
	bSizer611->Fit( m_CrtYdFrontPanel );
	m_LayerListFlexGridSizer->Add( m_CrtYdFrontPanel, 1, wxEXPAND, 5 );
	
	m_CrtYdFrontStaticText = new wxStaticText( m_LayersListPanel, ID_CRTYDFRONTCHOICE, _("Off-board, testing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CrtYdFrontStaticText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_FabFrontName = new wxStaticText( m_LayersListPanel, ID_FABFRONTNAME, _("Fab_Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabFrontName, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_FabFrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontPanel->SetBackgroundColour( wxColour( 236, 233, 236 ) );
	
	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );
	
	m_FabFrontCheckBox = new wxCheckBox( m_FabFrontPanel, ID_FABFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontCheckBox->SetToolTip( _("If you want a fabrication layer for the front side of the board") );
	
	bSizer61->Add( m_FabFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_FabFrontPanel->SetSizer( bSizer61 );
	m_FabFrontPanel->Layout();
	bSizer61->Fit( m_FabFrontPanel );
	m_LayerListFlexGridSizer->Add( m_FabFrontPanel, 1, wxEXPAND, 5 );
	
	m_FabFrontStaticText = new wxStaticText( m_LayersListPanel, ID_FABFRONTCHOICE, _("Off-board, manufacturing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabFrontStaticText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_AdhesFrontName = new wxStaticText( m_LayersListPanel, ID_ADHESFRONTNAME, _("Adhes_Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesFrontName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_AdhesFrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontPanel->SetBackgroundColour( wxColour( 236, 233, 236 ) );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );
	
	m_AdhesFrontCheckBox = new wxCheckBox( m_AdhesFrontPanel, ID_ADHESFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontCheckBox->SetToolTip( _("If you want an adhesive template for the front side of the board") );
	
	bSizer6->Add( m_AdhesFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_AdhesFrontPanel->SetSizer( bSizer6 );
	m_AdhesFrontPanel->Layout();
	bSizer6->Fit( m_AdhesFrontPanel );
	m_LayerListFlexGridSizer->Add( m_AdhesFrontPanel, 1, wxEXPAND, 5 );
	
	m_AdhesFrontStaticText = new wxStaticText( m_LayersListPanel, ID_ADHESFRONTCHOICE, _("Off-board, manufacturing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_SoldPFrontName = new wxStaticText( m_LayersListPanel, ID_SOLDPFRONTNAME, _("SoldP_Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPFrontName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SoldPFrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontPanel->SetBackgroundColour( wxColour( 255, 253, 235 ) );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );
	
	m_SoldPFrontCheckBox = new wxCheckBox( m_SoldPFrontPanel, ID_SOLDPFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontCheckBox->SetToolTip( _("If you want a solder paster layer for front side of the board") );
	
	bSizer7->Add( m_SoldPFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_SoldPFrontPanel->SetSizer( bSizer7 );
	m_SoldPFrontPanel->Layout();
	bSizer7->Fit( m_SoldPFrontPanel );
	m_LayerListFlexGridSizer->Add( m_SoldPFrontPanel, 1, wxEXPAND, 5 );
	
	m_SoldPFrontStaticText = new wxStaticText( m_LayersListPanel, ID_SOLDPFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SilkSFrontName = new wxStaticText( m_LayersListPanel, ID_SILKSFRONTNAME, _("SilkS_Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSFrontName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SilkSFrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_SilkSFrontPanel->SetBackgroundColour( wxColour( 255, 252, 235 ) );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_SilkSFrontCheckBox = new wxCheckBox( m_SilkSFrontPanel, ID_SILKSFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSFrontCheckBox->SetToolTip( _("If you want a silk screen layer for the front side of the board") );
	
	bSizer5->Add( m_SilkSFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_SilkSFrontPanel->SetSizer( bSizer5 );
	m_SilkSFrontPanel->Layout();
	bSizer5->Fit( m_SilkSFrontPanel );
	m_LayerListFlexGridSizer->Add( m_SilkSFrontPanel, 1, wxEXPAND, 5 );
	
	m_SilkSFrontStaticText = new wxStaticText( m_LayersListPanel, ID_SILKSFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_MaskFrontName = new wxStaticText( m_LayersListPanel, ID_MASKFRONTNAME, _("Mask_Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskFrontName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_MaskFrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MaskFrontPanel->SetBackgroundColour( wxColour( 255, 252, 235 ) );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	m_MaskFrontCheckBox = new wxCheckBox( m_MaskFrontPanel, ID_MASKFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskFrontCheckBox->SetToolTip( _("If you want a solder mask layer for the front of the board") );
	
	bSizer8->Add( m_MaskFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_MaskFrontPanel->SetSizer( bSizer8 );
	m_MaskFrontPanel->Layout();
	bSizer8->Fit( m_MaskFrontPanel );
	m_LayerListFlexGridSizer->Add( m_MaskFrontPanel, 1, wxEXPAND, 5 );
	
	m_MaskFrontStaticText = new wxStaticText( m_LayersListPanel, ID_MASKFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_FrontName = new wxTextCtrl( m_LayersListPanel, ID_FRONTNAME, _("Front_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontName->SetMaxLength( 20 ); 
	m_FrontName->SetToolTip( _("Layer name of front (top) copper layer") );
	
	m_LayerListFlexGridSizer->Add( m_FrontName, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_FrontPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_FrontPanel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_FrontCheckBox = new wxCheckBox( m_FrontPanel, ID_FRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontCheckBox->Enable( false );
	m_FrontCheckBox->SetToolTip( _("If you want a front copper layer") );
	
	bSizer9->Add( m_FrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_FrontPanel->SetSizer( bSizer9 );
	m_FrontPanel->Layout();
	bSizer9->Fit( m_FrontPanel );
	m_LayerListFlexGridSizer->Add( m_FrontPanel, 1, wxEXPAND, 5 );
	
	wxString m_FrontChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_FrontChoiceNChoices = sizeof( m_FrontChoiceChoices ) / sizeof( wxString );
	m_FrontChoice = new wxChoice( m_LayersListPanel, ID_FRONTCHOICE, wxDefaultPosition, wxDefaultSize, m_FrontChoiceNChoices, m_FrontChoiceChoices, 0 );
	m_FrontChoice->SetSelection( 0 );
	m_FrontChoice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_FrontChoice, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_In1Name = new wxTextCtrl( m_LayersListPanel, ID_IN1NAME, _("In1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In1Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In1Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In1Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In1Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn1Sizer;
	bIn1Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In1CheckBox = new wxCheckBox( m_In1Panel, ID_IN1CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In1CheckBox->Enable( false );
	
	bIn1Sizer->Add( m_In1CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In1Panel->SetSizer( bIn1Sizer );
	m_In1Panel->Layout();
	bIn1Sizer->Fit( m_In1Panel );
	m_LayerListFlexGridSizer->Add( m_In1Panel, 1, wxEXPAND, 5 );
	
	wxString m_In1ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In1ChoiceNChoices = sizeof( m_In1ChoiceChoices ) / sizeof( wxString );
	m_In1Choice = new wxChoice( m_LayersListPanel, ID_IN1CHOICE, wxDefaultPosition, wxDefaultSize, m_In1ChoiceNChoices, m_In1ChoiceChoices, 0 );
	m_In1Choice->SetSelection( 0 );
	m_In1Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In1Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In2Name = new wxTextCtrl( m_LayersListPanel, ID_IN2NAME, _("In2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In2Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In2Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In2Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In2Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn2Sizer;
	bIn2Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In2CheckBox = new wxCheckBox( m_In2Panel, ID_IN2CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In2CheckBox->Enable( false );
	
	bIn2Sizer->Add( m_In2CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In2Panel->SetSizer( bIn2Sizer );
	m_In2Panel->Layout();
	bIn2Sizer->Fit( m_In2Panel );
	m_LayerListFlexGridSizer->Add( m_In2Panel, 1, wxEXPAND, 5 );
	
	wxString m_In2ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In2ChoiceNChoices = sizeof( m_In2ChoiceChoices ) / sizeof( wxString );
	m_In2Choice = new wxChoice( m_LayersListPanel, ID_IN2CHOICE, wxDefaultPosition, wxDefaultSize, m_In2ChoiceNChoices, m_In2ChoiceChoices, 0 );
	m_In2Choice->SetSelection( 0 );
	m_In2Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In2Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In3Name = new wxTextCtrl( m_LayersListPanel, ID_IN3NAME, _("In3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In3Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In3Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In3Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In3Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn3Sizer;
	bIn3Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In3CheckBox = new wxCheckBox( m_In3Panel, ID_IN3CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In3CheckBox->Enable( false );
	
	bIn3Sizer->Add( m_In3CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In3Panel->SetSizer( bIn3Sizer );
	m_In3Panel->Layout();
	bIn3Sizer->Fit( m_In3Panel );
	m_LayerListFlexGridSizer->Add( m_In3Panel, 1, wxEXPAND, 5 );
	
	wxString m_In3ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In3ChoiceNChoices = sizeof( m_In3ChoiceChoices ) / sizeof( wxString );
	m_In3Choice = new wxChoice( m_LayersListPanel, ID_IN3CHOICE, wxDefaultPosition, wxDefaultSize, m_In3ChoiceNChoices, m_In3ChoiceChoices, 0 );
	m_In3Choice->SetSelection( 0 );
	m_In3Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In3Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In4Name = new wxTextCtrl( m_LayersListPanel, ID_IN4NAME, _("In4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In4Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In4Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In4Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In4Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn4Sizer;
	bIn4Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In4CheckBox = new wxCheckBox( m_In4Panel, ID_IN4CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In4CheckBox->Enable( false );
	
	bIn4Sizer->Add( m_In4CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In4Panel->SetSizer( bIn4Sizer );
	m_In4Panel->Layout();
	bIn4Sizer->Fit( m_In4Panel );
	m_LayerListFlexGridSizer->Add( m_In4Panel, 1, wxEXPAND, 5 );
	
	wxString m_In4ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In4ChoiceNChoices = sizeof( m_In4ChoiceChoices ) / sizeof( wxString );
	m_In4Choice = new wxChoice( m_LayersListPanel, ID_IN4CHOICE, wxDefaultPosition, wxDefaultSize, m_In4ChoiceNChoices, m_In4ChoiceChoices, 0 );
	m_In4Choice->SetSelection( 0 );
	m_In4Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In4Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In5Name = new wxTextCtrl( m_LayersListPanel, ID_IN5NAME, _("In5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In5Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In5Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In5Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In5Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn5Sizer;
	bIn5Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In5CheckBox = new wxCheckBox( m_In5Panel, ID_IN5CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In5CheckBox->Enable( false );
	
	bIn5Sizer->Add( m_In5CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In5Panel->SetSizer( bIn5Sizer );
	m_In5Panel->Layout();
	bIn5Sizer->Fit( m_In5Panel );
	m_LayerListFlexGridSizer->Add( m_In5Panel, 1, wxEXPAND, 5 );
	
	wxString m_In5ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In5ChoiceNChoices = sizeof( m_In5ChoiceChoices ) / sizeof( wxString );
	m_In5Choice = new wxChoice( m_LayersListPanel, ID_IN5CHOICE, wxDefaultPosition, wxDefaultSize, m_In5ChoiceNChoices, m_In5ChoiceChoices, 0 );
	m_In5Choice->SetSelection( 0 );
	m_In5Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In5Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In6Name = new wxTextCtrl( m_LayersListPanel, ID_IN6NAME, _("In6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In6Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In6Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In6Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In6Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn6Sizer;
	bIn6Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In6CheckBox = new wxCheckBox( m_In6Panel, ID_IN6CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In6CheckBox->Enable( false );
	
	bIn6Sizer->Add( m_In6CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In6Panel->SetSizer( bIn6Sizer );
	m_In6Panel->Layout();
	bIn6Sizer->Fit( m_In6Panel );
	m_LayerListFlexGridSizer->Add( m_In6Panel, 1, wxEXPAND, 5 );
	
	wxString m_In6ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In6ChoiceNChoices = sizeof( m_In6ChoiceChoices ) / sizeof( wxString );
	m_In6Choice = new wxChoice( m_LayersListPanel, ID_IN6CHOICE, wxDefaultPosition, wxDefaultSize, m_In6ChoiceNChoices, m_In6ChoiceChoices, 0 );
	m_In6Choice->SetSelection( 0 );
	m_In6Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In6Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In7Name = new wxTextCtrl( m_LayersListPanel, ID_IN7NAME, _("In7"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In7Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In7Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In7Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In7Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn7Sizer;
	bIn7Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In7CheckBox = new wxCheckBox( m_In7Panel, ID_IN7CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In7CheckBox->Enable( false );
	
	bIn7Sizer->Add( m_In7CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In7Panel->SetSizer( bIn7Sizer );
	m_In7Panel->Layout();
	bIn7Sizer->Fit( m_In7Panel );
	m_LayerListFlexGridSizer->Add( m_In7Panel, 1, wxEXPAND, 5 );
	
	wxString m_In7ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In7ChoiceNChoices = sizeof( m_In7ChoiceChoices ) / sizeof( wxString );
	m_In7Choice = new wxChoice( m_LayersListPanel, ID_IN7CHOICE, wxDefaultPosition, wxDefaultSize, m_In7ChoiceNChoices, m_In7ChoiceChoices, 0 );
	m_In7Choice->SetSelection( 0 );
	m_In7Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In7Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In8Name = new wxTextCtrl( m_LayersListPanel, ID_IN8NAME, _("In8"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In8Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In8Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In8Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In8Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn8Sizer;
	bIn8Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In8CheckBox = new wxCheckBox( m_In8Panel, ID_IN8CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In8CheckBox->Enable( false );
	
	bIn8Sizer->Add( m_In8CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In8Panel->SetSizer( bIn8Sizer );
	m_In8Panel->Layout();
	bIn8Sizer->Fit( m_In8Panel );
	m_LayerListFlexGridSizer->Add( m_In8Panel, 1, wxEXPAND, 5 );
	
	wxString m_In8ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In8ChoiceNChoices = sizeof( m_In8ChoiceChoices ) / sizeof( wxString );
	m_In8Choice = new wxChoice( m_LayersListPanel, ID_IN8CHOICE, wxDefaultPosition, wxDefaultSize, m_In8ChoiceNChoices, m_In8ChoiceChoices, 0 );
	m_In8Choice->SetSelection( 0 );
	m_In8Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In8Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In9Name = new wxTextCtrl( m_LayersListPanel, ID_IN9NAME, _("In9"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In9Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In9Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In9Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In9Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn9Sizer;
	bIn9Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In9CheckBox = new wxCheckBox( m_In9Panel, ID_IN9CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In9CheckBox->Enable( false );
	
	bIn9Sizer->Add( m_In9CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In9Panel->SetSizer( bIn9Sizer );
	m_In9Panel->Layout();
	bIn9Sizer->Fit( m_In9Panel );
	m_LayerListFlexGridSizer->Add( m_In9Panel, 1, wxEXPAND, 5 );
	
	wxString m_In9ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In9ChoiceNChoices = sizeof( m_In9ChoiceChoices ) / sizeof( wxString );
	m_In9Choice = new wxChoice( m_LayersListPanel, ID_IN9CHOICE, wxDefaultPosition, wxDefaultSize, m_In9ChoiceNChoices, m_In9ChoiceChoices, 0 );
	m_In9Choice->SetSelection( 0 );
	m_In9Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In9Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In10Name = new wxTextCtrl( m_LayersListPanel, ID_IN10NAME, _("In10"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In10Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In10Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In10Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In10Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn10Sizer;
	bIn10Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In10CheckBox = new wxCheckBox( m_In10Panel, ID_IN10CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In10CheckBox->Enable( false );
	
	bIn10Sizer->Add( m_In10CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In10Panel->SetSizer( bIn10Sizer );
	m_In10Panel->Layout();
	bIn10Sizer->Fit( m_In10Panel );
	m_LayerListFlexGridSizer->Add( m_In10Panel, 1, wxEXPAND, 5 );
	
	wxString m_In10ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In10ChoiceNChoices = sizeof( m_In10ChoiceChoices ) / sizeof( wxString );
	m_In10Choice = new wxChoice( m_LayersListPanel, ID_IN10CHOICE, wxDefaultPosition, wxDefaultSize, m_In10ChoiceNChoices, m_In10ChoiceChoices, 0 );
	m_In10Choice->SetSelection( 0 );
	m_In10Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In10Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In11Name = new wxTextCtrl( m_LayersListPanel, ID_IN11NAME, _("In11"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In11Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In11Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In11Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In11Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn11Sizer;
	bIn11Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In11CheckBox = new wxCheckBox( m_In11Panel, ID_IN11CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In11CheckBox->Enable( false );
	
	bIn11Sizer->Add( m_In11CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In11Panel->SetSizer( bIn11Sizer );
	m_In11Panel->Layout();
	bIn11Sizer->Fit( m_In11Panel );
	m_LayerListFlexGridSizer->Add( m_In11Panel, 1, wxEXPAND, 5 );
	
	wxString m_In11ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In11ChoiceNChoices = sizeof( m_In11ChoiceChoices ) / sizeof( wxString );
	m_In11Choice = new wxChoice( m_LayersListPanel, ID_IN11CHOICE, wxDefaultPosition, wxDefaultSize, m_In11ChoiceNChoices, m_In11ChoiceChoices, 0 );
	m_In11Choice->SetSelection( 0 );
	m_In11Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In11Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In12Name = new wxTextCtrl( m_LayersListPanel, ID_IN12NAME, _("In12"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In12Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In12Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In12Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In12Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn12Sizer;
	bIn12Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In12CheckBox = new wxCheckBox( m_In12Panel, ID_IN12CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In12CheckBox->Enable( false );
	
	bIn12Sizer->Add( m_In12CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In12Panel->SetSizer( bIn12Sizer );
	m_In12Panel->Layout();
	bIn12Sizer->Fit( m_In12Panel );
	m_LayerListFlexGridSizer->Add( m_In12Panel, 1, wxEXPAND, 5 );
	
	wxString m_In12ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In12ChoiceNChoices = sizeof( m_In12ChoiceChoices ) / sizeof( wxString );
	m_In12Choice = new wxChoice( m_LayersListPanel, ID_IN12CHOICE, wxDefaultPosition, wxDefaultSize, m_In12ChoiceNChoices, m_In12ChoiceChoices, 0 );
	m_In12Choice->SetSelection( 0 );
	m_In12Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In12Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In13Name = new wxTextCtrl( m_LayersListPanel, ID_IN13NAME, _("In13"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In13Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In13Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In13Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In13Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn13Sizer;
	bIn13Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In13CheckBox = new wxCheckBox( m_In13Panel, ID_IN13CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In13CheckBox->Enable( false );
	
	bIn13Sizer->Add( m_In13CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In13Panel->SetSizer( bIn13Sizer );
	m_In13Panel->Layout();
	bIn13Sizer->Fit( m_In13Panel );
	m_LayerListFlexGridSizer->Add( m_In13Panel, 1, wxEXPAND, 5 );
	
	wxString m_In13ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In13ChoiceNChoices = sizeof( m_In13ChoiceChoices ) / sizeof( wxString );
	m_In13Choice = new wxChoice( m_LayersListPanel, ID_IN13CHOICE, wxDefaultPosition, wxDefaultSize, m_In13ChoiceNChoices, m_In13ChoiceChoices, 0 );
	m_In13Choice->SetSelection( 0 );
	m_In13Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In13Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In14Name = new wxTextCtrl( m_LayersListPanel, ID_IN14NAME, _("In14"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In14Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In14Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In14Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In14Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn14Sizer;
	bIn14Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In14CheckBox = new wxCheckBox( m_In14Panel, ID_IN14CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In14CheckBox->Enable( false );
	
	bIn14Sizer->Add( m_In14CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In14Panel->SetSizer( bIn14Sizer );
	m_In14Panel->Layout();
	bIn14Sizer->Fit( m_In14Panel );
	m_LayerListFlexGridSizer->Add( m_In14Panel, 1, wxEXPAND, 5 );
	
	wxString m_In14ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In14ChoiceNChoices = sizeof( m_In14ChoiceChoices ) / sizeof( wxString );
	m_In14Choice = new wxChoice( m_LayersListPanel, ID_IN14CHOICE, wxDefaultPosition, wxDefaultSize, m_In14ChoiceNChoices, m_In14ChoiceChoices, 0 );
	m_In14Choice->SetSelection( 0 );
	m_In14Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In14Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In15Name = new wxTextCtrl( m_LayersListPanel, ID_IN15NAME, _("In15"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In15Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In15Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In15Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In15Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn15Sizer;
	bIn15Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In15CheckBox = new wxCheckBox( m_In15Panel, ID_IN15CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In15CheckBox->Enable( false );
	
	bIn15Sizer->Add( m_In15CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In15Panel->SetSizer( bIn15Sizer );
	m_In15Panel->Layout();
	bIn15Sizer->Fit( m_In15Panel );
	m_LayerListFlexGridSizer->Add( m_In15Panel, 1, wxEXPAND, 5 );
	
	wxString m_In15ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In15ChoiceNChoices = sizeof( m_In15ChoiceChoices ) / sizeof( wxString );
	m_In15Choice = new wxChoice( m_LayersListPanel, ID_IN15CHOICE, wxDefaultPosition, wxDefaultSize, m_In15ChoiceNChoices, m_In15ChoiceChoices, 0 );
	m_In15Choice->SetSelection( 0 );
	m_In15Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In15Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In16Name = new wxTextCtrl( m_LayersListPanel, ID_IN16NAME, _("In16"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In16Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In16Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In16Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In16Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn16Sizer;
	bIn16Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In16CheckBox = new wxCheckBox( m_In16Panel, ID_IN16CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In16CheckBox->Enable( false );
	
	bIn16Sizer->Add( m_In16CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In16Panel->SetSizer( bIn16Sizer );
	m_In16Panel->Layout();
	bIn16Sizer->Fit( m_In16Panel );
	m_LayerListFlexGridSizer->Add( m_In16Panel, 1, wxEXPAND, 5 );
	
	wxString m_In16ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In16ChoiceNChoices = sizeof( m_In16ChoiceChoices ) / sizeof( wxString );
	m_In16Choice = new wxChoice( m_LayersListPanel, ID_IN16CHOICE, wxDefaultPosition, wxDefaultSize, m_In16ChoiceNChoices, m_In16ChoiceChoices, 0 );
	m_In16Choice->SetSelection( 0 );
	m_In16Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In16Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In17Name = new wxTextCtrl( m_LayersListPanel, ID_IN17NAME, _("In17"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In17Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In17Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In17Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In17Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn17Sizer;
	bIn17Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In17CheckBox = new wxCheckBox( m_In17Panel, ID_IN17CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In17CheckBox->Enable( false );
	
	bIn17Sizer->Add( m_In17CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In17Panel->SetSizer( bIn17Sizer );
	m_In17Panel->Layout();
	bIn17Sizer->Fit( m_In17Panel );
	m_LayerListFlexGridSizer->Add( m_In17Panel, 1, wxEXPAND, 5 );
	
	wxString m_In17ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In17ChoiceNChoices = sizeof( m_In17ChoiceChoices ) / sizeof( wxString );
	m_In17Choice = new wxChoice( m_LayersListPanel, ID_IN17CHOICE, wxDefaultPosition, wxDefaultSize, m_In17ChoiceNChoices, m_In17ChoiceChoices, 0 );
	m_In17Choice->SetSelection( 0 );
	m_In17Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In17Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In18Name = new wxTextCtrl( m_LayersListPanel, ID_IN18NAME, _("In18"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In18Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In18Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In18Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In18Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn18Sizer;
	bIn18Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In18CheckBox = new wxCheckBox( m_In18Panel, ID_IN18CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In18CheckBox->Enable( false );
	
	bIn18Sizer->Add( m_In18CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In18Panel->SetSizer( bIn18Sizer );
	m_In18Panel->Layout();
	bIn18Sizer->Fit( m_In18Panel );
	m_LayerListFlexGridSizer->Add( m_In18Panel, 1, wxEXPAND, 5 );
	
	wxString m_In18ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In18ChoiceNChoices = sizeof( m_In18ChoiceChoices ) / sizeof( wxString );
	m_In18Choice = new wxChoice( m_LayersListPanel, ID_IN18CHOICE, wxDefaultPosition, wxDefaultSize, m_In18ChoiceNChoices, m_In18ChoiceChoices, 0 );
	m_In18Choice->SetSelection( 0 );
	m_In18Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In18Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In19Name = new wxTextCtrl( m_LayersListPanel, ID_IN19NAME, _("In19"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In19Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In19Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In19Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In19Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn19Sizer;
	bIn19Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In19CheckBox = new wxCheckBox( m_In19Panel, ID_IN19CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In19CheckBox->Enable( false );
	
	bIn19Sizer->Add( m_In19CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In19Panel->SetSizer( bIn19Sizer );
	m_In19Panel->Layout();
	bIn19Sizer->Fit( m_In19Panel );
	m_LayerListFlexGridSizer->Add( m_In19Panel, 1, wxEXPAND, 5 );
	
	wxString m_In19ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In19ChoiceNChoices = sizeof( m_In19ChoiceChoices ) / sizeof( wxString );
	m_In19Choice = new wxChoice( m_LayersListPanel, ID_IN19CHOICE, wxDefaultPosition, wxDefaultSize, m_In19ChoiceNChoices, m_In19ChoiceChoices, 0 );
	m_In19Choice->SetSelection( 0 );
	m_In19Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In19Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In20Name = new wxTextCtrl( m_LayersListPanel, ID_IN20NAME, _("In20"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In20Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In20Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In20Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In20Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn20Sizer;
	bIn20Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In20CheckBox = new wxCheckBox( m_In20Panel, ID_IN20CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In20CheckBox->Enable( false );
	
	bIn20Sizer->Add( m_In20CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In20Panel->SetSizer( bIn20Sizer );
	m_In20Panel->Layout();
	bIn20Sizer->Fit( m_In20Panel );
	m_LayerListFlexGridSizer->Add( m_In20Panel, 1, wxEXPAND, 5 );
	
	wxString m_In20ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In20ChoiceNChoices = sizeof( m_In20ChoiceChoices ) / sizeof( wxString );
	m_In20Choice = new wxChoice( m_LayersListPanel, ID_IN20CHOICE, wxDefaultPosition, wxDefaultSize, m_In20ChoiceNChoices, m_In20ChoiceChoices, 0 );
	m_In20Choice->SetSelection( 0 );
	m_In20Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In20Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In21Name = new wxTextCtrl( m_LayersListPanel, ID_IN21NAME, _("In21"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In21Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In21Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In21Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In21Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn21Sizer;
	bIn21Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In21CheckBox = new wxCheckBox( m_In21Panel, ID_IN21CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In21CheckBox->Enable( false );
	
	bIn21Sizer->Add( m_In21CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In21Panel->SetSizer( bIn21Sizer );
	m_In21Panel->Layout();
	bIn21Sizer->Fit( m_In21Panel );
	m_LayerListFlexGridSizer->Add( m_In21Panel, 1, wxEXPAND, 5 );
	
	wxString m_In21ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In21ChoiceNChoices = sizeof( m_In21ChoiceChoices ) / sizeof( wxString );
	m_In21Choice = new wxChoice( m_LayersListPanel, ID_IN21CHOICE, wxDefaultPosition, wxDefaultSize, m_In21ChoiceNChoices, m_In21ChoiceChoices, 0 );
	m_In21Choice->SetSelection( 0 );
	m_In21Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In21Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In22Name = new wxTextCtrl( m_LayersListPanel, ID_IN22NAME, _("In22"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In22Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In22Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In22Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In22Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn22Sizer;
	bIn22Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In22CheckBox = new wxCheckBox( m_In22Panel, ID_IN22CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In22CheckBox->Enable( false );
	
	bIn22Sizer->Add( m_In22CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In22Panel->SetSizer( bIn22Sizer );
	m_In22Panel->Layout();
	bIn22Sizer->Fit( m_In22Panel );
	m_LayerListFlexGridSizer->Add( m_In22Panel, 1, wxEXPAND, 5 );
	
	wxString m_In22ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In22ChoiceNChoices = sizeof( m_In22ChoiceChoices ) / sizeof( wxString );
	m_In22Choice = new wxChoice( m_LayersListPanel, ID_IN22CHOICE, wxDefaultPosition, wxDefaultSize, m_In22ChoiceNChoices, m_In22ChoiceChoices, 0 );
	m_In22Choice->SetSelection( 0 );
	m_In22Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In22Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In23Name = new wxTextCtrl( m_LayersListPanel, ID_IN23NAME, _("In23"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In23Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In23Name, 0, wxALL|wxEXPAND, 5 );
	
	m_In23Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In23Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn23Sizer;
	bIn23Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In23CheckBox = new wxCheckBox( m_In23Panel, ID_IN23CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In23CheckBox->Enable( false );
	
	bIn23Sizer->Add( m_In23CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In23Panel->SetSizer( bIn23Sizer );
	m_In23Panel->Layout();
	bIn23Sizer->Fit( m_In23Panel );
	m_LayerListFlexGridSizer->Add( m_In23Panel, 1, wxEXPAND, 5 );
	
	wxString m_In23ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In23ChoiceNChoices = sizeof( m_In23ChoiceChoices ) / sizeof( wxString );
	m_In23Choice = new wxChoice( m_LayersListPanel, ID_IN22CHOICE, wxDefaultPosition, wxDefaultSize, m_In23ChoiceNChoices, m_In23ChoiceChoices, 0 );
	m_In23Choice->SetSelection( 0 );
	m_In23Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In23Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In24Name = new wxTextCtrl( m_LayersListPanel, ID_IN24NAME, _("In24"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In24Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In24Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In24Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In24Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn24Sizer;
	bIn24Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In24CheckBox = new wxCheckBox( m_In24Panel, ID_IN24CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In24CheckBox->Enable( false );
	
	bIn24Sizer->Add( m_In24CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In24Panel->SetSizer( bIn24Sizer );
	m_In24Panel->Layout();
	bIn24Sizer->Fit( m_In24Panel );
	m_LayerListFlexGridSizer->Add( m_In24Panel, 1, wxEXPAND, 5 );
	
	wxString m_In24ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In24ChoiceNChoices = sizeof( m_In24ChoiceChoices ) / sizeof( wxString );
	m_In24Choice = new wxChoice( m_LayersListPanel, ID_IN24CHOICE, wxDefaultPosition, wxDefaultSize, m_In24ChoiceNChoices, m_In24ChoiceChoices, 0 );
	m_In24Choice->SetSelection( 0 );
	m_In24Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In24Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In25Name = new wxTextCtrl( m_LayersListPanel, ID_IN25NAME, _("In25"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In25Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In25Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In25Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In25Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn25Sizer;
	bIn25Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In25CheckBox = new wxCheckBox( m_In25Panel, ID_IN25CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In25CheckBox->Enable( false );
	
	bIn25Sizer->Add( m_In25CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In25Panel->SetSizer( bIn25Sizer );
	m_In25Panel->Layout();
	bIn25Sizer->Fit( m_In25Panel );
	m_LayerListFlexGridSizer->Add( m_In25Panel, 1, wxEXPAND, 5 );
	
	wxString m_In25ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In25ChoiceNChoices = sizeof( m_In25ChoiceChoices ) / sizeof( wxString );
	m_In25Choice = new wxChoice( m_LayersListPanel, ID_IN25CHOICE, wxDefaultPosition, wxDefaultSize, m_In25ChoiceNChoices, m_In25ChoiceChoices, 0 );
	m_In25Choice->SetSelection( 0 );
	m_In25Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In25Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In26Name = new wxTextCtrl( m_LayersListPanel, ID_IN26NAME, _("In26"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In26Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In26Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In26Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In26Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn26Sizer;
	bIn26Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In26CheckBox = new wxCheckBox( m_In26Panel, ID_IN26CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In26CheckBox->Enable( false );
	
	bIn26Sizer->Add( m_In26CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In26Panel->SetSizer( bIn26Sizer );
	m_In26Panel->Layout();
	bIn26Sizer->Fit( m_In26Panel );
	m_LayerListFlexGridSizer->Add( m_In26Panel, 1, wxEXPAND, 5 );
	
	wxString m_In26ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In26ChoiceNChoices = sizeof( m_In26ChoiceChoices ) / sizeof( wxString );
	m_In26Choice = new wxChoice( m_LayersListPanel, ID_IN26CHOICE, wxDefaultPosition, wxDefaultSize, m_In26ChoiceNChoices, m_In26ChoiceChoices, 0 );
	m_In26Choice->SetSelection( 0 );
	m_In26Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In26Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In27Name = new wxTextCtrl( m_LayersListPanel, ID_IN27NAME, _("In27"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In27Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In27Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In27Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In27Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn27Sizer;
	bIn27Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In27CheckBox = new wxCheckBox( m_In27Panel, ID_IN27CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In27CheckBox->Enable( false );
	
	bIn27Sizer->Add( m_In27CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In27Panel->SetSizer( bIn27Sizer );
	m_In27Panel->Layout();
	bIn27Sizer->Fit( m_In27Panel );
	m_LayerListFlexGridSizer->Add( m_In27Panel, 1, wxEXPAND, 5 );
	
	wxString m_In27ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In27ChoiceNChoices = sizeof( m_In27ChoiceChoices ) / sizeof( wxString );
	m_In27Choice = new wxChoice( m_LayersListPanel, ID_IN27CHOICE, wxDefaultPosition, wxDefaultSize, m_In27ChoiceNChoices, m_In27ChoiceChoices, 0 );
	m_In27Choice->SetSelection( 0 );
	m_In27Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In27Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In28Name = new wxTextCtrl( m_LayersListPanel, ID_IN28NAME, _("In28"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In28Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In28Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In28Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In28Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn28Sizer;
	bIn28Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In28CheckBox = new wxCheckBox( m_In28Panel, ID_IN28CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In28CheckBox->Enable( false );
	
	bIn28Sizer->Add( m_In28CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In28Panel->SetSizer( bIn28Sizer );
	m_In28Panel->Layout();
	bIn28Sizer->Fit( m_In28Panel );
	m_LayerListFlexGridSizer->Add( m_In28Panel, 1, wxEXPAND, 5 );
	
	wxString m_In28ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In28ChoiceNChoices = sizeof( m_In28ChoiceChoices ) / sizeof( wxString );
	m_In28Choice = new wxChoice( m_LayersListPanel, ID_IN28CHOICE, wxDefaultPosition, wxDefaultSize, m_In28ChoiceNChoices, m_In28ChoiceChoices, 0 );
	m_In28Choice->SetSelection( 0 );
	m_In28Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In28Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In29Name = new wxTextCtrl( m_LayersListPanel, ID_IN29NAME, _("In29"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In29Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In29Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In29Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In29Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn29Sizer;
	bIn29Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In29CheckBox = new wxCheckBox( m_In29Panel, ID_IN29CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In29CheckBox->Enable( false );
	
	bIn29Sizer->Add( m_In29CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In29Panel->SetSizer( bIn29Sizer );
	m_In29Panel->Layout();
	bIn29Sizer->Fit( m_In29Panel );
	m_LayerListFlexGridSizer->Add( m_In29Panel, 1, wxEXPAND, 5 );
	
	wxString m_In29ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In29ChoiceNChoices = sizeof( m_In29ChoiceChoices ) / sizeof( wxString );
	m_In29Choice = new wxChoice( m_LayersListPanel, ID_IN29CHOICE, wxDefaultPosition, wxDefaultSize, m_In29ChoiceNChoices, m_In29ChoiceChoices, 0 );
	m_In29Choice->SetSelection( 0 );
	m_In29Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In29Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In30Name = new wxTextCtrl( m_LayersListPanel, ID_IN30NAME, _("In30"), wxDefaultPosition, wxDefaultSize, 0 );
	m_In30Name->SetMaxLength( 20 ); 
	m_LayerListFlexGridSizer->Add( m_In30Name, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_In30Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_In30Panel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bIn30Sizer;
	bIn30Sizer = new wxBoxSizer( wxVERTICAL );
	
	m_In30CheckBox = new wxCheckBox( m_In30Panel, ID_IN30CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_In30CheckBox->Enable( false );
	
	bIn30Sizer->Add( m_In30CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_In30Panel->SetSizer( bIn30Sizer );
	m_In30Panel->Layout();
	bIn30Sizer->Fit( m_In30Panel );
	m_LayerListFlexGridSizer->Add( m_In30Panel, 1, wxEXPAND, 5 );
	
	wxString m_In30ChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_In30ChoiceNChoices = sizeof( m_In30ChoiceChoices ) / sizeof( wxString );
	m_In30Choice = new wxChoice( m_LayersListPanel, ID_IN30CHOICE, wxDefaultPosition, wxDefaultSize, m_In30ChoiceNChoices, m_In30ChoiceChoices, 0 );
	m_In30Choice->SetSelection( 0 );
	m_In30Choice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_In30Choice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_BackName = new wxTextCtrl( m_LayersListPanel, ID_BACKNAME, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BackName->SetMaxLength( 20 ); 
	m_BackName->SetToolTip( _("Layer name of back (bottom) copper layer") );
	
	m_LayerListFlexGridSizer->Add( m_BackName, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_BackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_BackPanel->SetBackgroundColour( wxColour( 236, 253, 216 ) );
	
	wxBoxSizer* bBackSizer;
	bBackSizer = new wxBoxSizer( wxVERTICAL );
	
	m_BackCheckBox = new wxCheckBox( m_BackPanel, ID_BACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_BackCheckBox->Enable( false );
	m_BackCheckBox->SetToolTip( _("If you want a back copper layer") );
	
	bBackSizer->Add( m_BackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_BackPanel->SetSizer( bBackSizer );
	m_BackPanel->Layout();
	bBackSizer->Fit( m_BackPanel );
	m_LayerListFlexGridSizer->Add( m_BackPanel, 1, wxEXPAND, 5 );
	
	wxString m_BackChoiceChoices[] = { _("signal"), _("power"), _("mixed"), _("jumper") };
	int m_BackChoiceNChoices = sizeof( m_BackChoiceChoices ) / sizeof( wxString );
	m_BackChoice = new wxChoice( m_LayersListPanel, ID_BACKCHOICE, wxDefaultPosition, wxDefaultSize, m_BackChoiceNChoices, m_BackChoiceChoices, 0 );
	m_BackChoice->SetSelection( 0 );
	m_BackChoice->SetToolTip( _("Copper layer type for Freerouter.  Power layers are removed from Freerouter's layer menus.") );
	
	m_LayerListFlexGridSizer->Add( m_BackChoice, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND, 5 );
	
	m_MaskBackName = new wxStaticText( m_LayersListPanel, ID_MASKBACKNAME, _("Mask_Back_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskBackName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_MaskBackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackPanel->SetBackgroundColour( wxColour( 255, 252, 235 ) );
	
	wxBoxSizer* bSizer24;
	bSizer24 = new wxBoxSizer( wxVERTICAL );
	
	m_MaskBackCheckBox = new wxCheckBox( m_MaskBackPanel, ID_MASKBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackCheckBox->SetToolTip( _("If you want a solder mask layer for the back side of the board") );
	
	bSizer24->Add( m_MaskBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_MaskBackPanel->SetSizer( bSizer24 );
	m_MaskBackPanel->Layout();
	bSizer24->Fit( m_MaskBackPanel );
	m_LayerListFlexGridSizer->Add( m_MaskBackPanel, 1, wxEXPAND, 5 );
	
	m_MaskBackStaticText = new wxStaticText( m_LayersListPanel, ID_MASKBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SilkSBackName = new wxStaticText( m_LayersListPanel, ID_SILKSBACKNAME, _("SilkS_Back_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSBackName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SilkSBackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackPanel->SetBackgroundColour( wxColour( 255, 252, 235 ) );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxVERTICAL );
	
	m_SilkSBackCheckBox = new wxCheckBox( m_SilkSBackPanel, ID_SILKSBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackCheckBox->SetToolTip( _("If you want a silk screen layer for the back side of the board") );
	
	bSizer25->Add( m_SilkSBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_SilkSBackPanel->SetSizer( bSizer25 );
	m_SilkSBackPanel->Layout();
	bSizer25->Fit( m_SilkSBackPanel );
	m_LayerListFlexGridSizer->Add( m_SilkSBackPanel, 1, wxEXPAND, 5 );
	
	m_SilkSBackStaticText = new wxStaticText( m_LayersListPanel, ID_SILKSBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SoldPBackName = new wxStaticText( m_LayersListPanel, ID_SOLDPBACKNAME, _("SoldP_Back_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPBackName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_SoldPBackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackPanel->SetBackgroundColour( wxColour( 255, 253, 235 ) );
	
	wxBoxSizer* bSizer26;
	bSizer26 = new wxBoxSizer( wxVERTICAL );
	
	m_SoldPBackCheckBox = new wxCheckBox( m_SoldPBackPanel, ID_SOLDPBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackCheckBox->SetToolTip( _("If you want a solder paste layer for the back side of the board") );
	
	bSizer26->Add( m_SoldPBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_SoldPBackPanel->SetSizer( bSizer26 );
	m_SoldPBackPanel->Layout();
	bSizer26->Fit( m_SoldPBackPanel );
	m_LayerListFlexGridSizer->Add( m_SoldPBackPanel, 1, wxEXPAND, 5 );
	
	m_SoldPBackStaticText = new wxStaticText( m_LayersListPanel, ID_SOLDPBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_AdhesBackName = new wxStaticText( m_LayersListPanel, ID_ADHESBACKNAME, _("Adhes_Back_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesBackName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_AdhesBackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackPanel->SetBackgroundColour( wxColour( 236, 233, 236 ) );
	
	wxBoxSizer* bSizer27;
	bSizer27 = new wxBoxSizer( wxVERTICAL );
	
	m_AdhesBackCheckBox = new wxCheckBox( m_AdhesBackPanel, ID_ADHESBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackCheckBox->SetToolTip( _("If you want an adhesive layer for the back side of the board") );
	
	bSizer27->Add( m_AdhesBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_AdhesBackPanel->SetSizer( bSizer27 );
	m_AdhesBackPanel->Layout();
	bSizer27->Fit( m_AdhesBackPanel );
	m_LayerListFlexGridSizer->Add( m_AdhesBackPanel, 1, wxEXPAND, 5 );
	
	m_AdhesBackStaticText = new wxStaticText( m_LayersListPanel, ID_ADHESBACKCHOICE, _("Off-board, manufacturing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_FabBackName = new wxStaticText( m_LayersListPanel, ID_FABBACKNAME, _("Fab_Back_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabBackName, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_FabBackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackPanel->SetBackgroundColour( wxColour( 236, 233, 236 ) );
	
	wxBoxSizer* bSizer281;
	bSizer281 = new wxBoxSizer( wxVERTICAL );
	
	m_FabBackCheckBox = new wxCheckBox( m_FabBackPanel, ID_FABBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackCheckBox->SetToolTip( _("If you want a fabrication layer for the back side of the board") );
	
	bSizer281->Add( m_FabBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_FabBackPanel->SetSizer( bSizer281 );
	m_FabBackPanel->Layout();
	bSizer281->Fit( m_FabBackPanel );
	m_LayerListFlexGridSizer->Add( m_FabBackPanel, 1, wxEXPAND, 5 );
	
	m_FabBackStaticText = new wxStaticText( m_LayersListPanel, ID_FABBACKCHOICE, _("Off-board, manufacturing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabBackStaticText, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_CrtYdBackName = new wxStaticText( m_LayersListPanel, ID_CRTYDBACKNAME, _("CrtYd_Back_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CrtYdBackName, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_CrtYdBackPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackPanel->SetBackgroundColour( wxColour( 255, 233, 236 ) );
	
	wxBoxSizer* bSizer6111;
	bSizer6111 = new wxBoxSizer( wxVERTICAL );
	
	m_CrtYdBackCheckBox = new wxCheckBox( m_CrtYdBackPanel, ID_CRTYDBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackCheckBox->SetToolTip( _("If you want a courtyard layer for the front side of the board") );
	
	bSizer6111->Add( m_CrtYdBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_CrtYdBackPanel->SetSizer( bSizer6111 );
	m_CrtYdBackPanel->Layout();
	bSizer6111->Fit( m_CrtYdBackPanel );
	m_LayerListFlexGridSizer->Add( m_CrtYdBackPanel, 1, wxEXPAND, 5 );
	
	m_CrtYdBackStaticText = new wxStaticText( m_LayersListPanel, ID_CRTYDBACKCHOICE, _("Off-board, testing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CrtYdBackStaticText, 0, wxALL, 5 );
	
	m_PCBEdgesName = new wxStaticText( m_LayersListPanel, ID_PCBEDGESNAME, _("PCB_Edges_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_PCBEdgesName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_PCBEdgesPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesPanel->SetBackgroundColour( wxColour( 255, 253, 216 ) );
	
	wxBoxSizer* bSizer28;
	bSizer28 = new wxBoxSizer( wxVERTICAL );
	
	m_PCBEdgesCheckBox = new wxCheckBox( m_PCBEdgesPanel, ID_PCBEDGESCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesCheckBox->SetToolTip( _("If you want a board perimeter layer") );
	
	bSizer28->Add( m_PCBEdgesCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_PCBEdgesPanel->SetSizer( bSizer28 );
	m_PCBEdgesPanel->Layout();
	bSizer28->Fit( m_PCBEdgesPanel );
	m_LayerListFlexGridSizer->Add( m_PCBEdgesPanel, 1, wxEXPAND, 5 );
	
	m_PCBEdgesStaticText = new wxStaticText( m_LayersListPanel, ID_PCBEDGESCHOICE, _("Board contour"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_PCBEdgesStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_MarginName = new wxStaticText( m_LayersListPanel, ID_MARGINNAME, _("Margin_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MarginName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MarginName, 0, wxALL, 5 );
	
	m_MarginPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MarginPanel->SetBackgroundColour( wxColour( 236, 233, 255 ) );
	
	wxBoxSizer* bSizer291;
	bSizer291 = new wxBoxSizer( wxVERTICAL );
	
	m_MarginCheckBox = new wxCheckBox( m_MarginPanel, ID_MARGINCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer291->Add( m_MarginCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_MarginPanel->SetSizer( bSizer291 );
	m_MarginPanel->Layout();
	bSizer291->Fit( m_MarginPanel );
	m_LayerListFlexGridSizer->Add( m_MarginPanel, 1, wxEXPAND, 5 );
	
	m_MarginStaticText = new wxStaticText( m_LayersListPanel, ID_ECO2CHOICE, _("Edge_Cuts setback"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MarginStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MarginStaticText, 0, wxALL, 5 );
	
	m_Eco1Name = new wxStaticText( m_LayersListPanel, ID_ECO2NAME, _("Eco1_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco1Name->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco1Name, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_Eco1Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Eco1Panel->SetBackgroundColour( wxColour( 236, 233, 255 ) );
	
	wxBoxSizer* bSizer29;
	bSizer29 = new wxBoxSizer( wxVERTICAL );
	
	m_Eco1CheckBox = new wxCheckBox( m_Eco1Panel, ID_ECO2CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer29->Add( m_Eco1CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_Eco1Panel->SetSizer( bSizer29 );
	m_Eco1Panel->Layout();
	bSizer29->Fit( m_Eco1Panel );
	m_LayerListFlexGridSizer->Add( m_Eco1Panel, 1, wxEXPAND, 5 );
	
	m_Eco1StaticText = new wxStaticText( m_LayersListPanel, ID_ECO2CHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco1StaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco1StaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_Eco2Name = new wxStaticText( m_LayersListPanel, ID_ECO1NAME, _("Eco2_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco2Name->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco2Name, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_Eco2Panel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_Eco2Panel->SetBackgroundColour( wxColour( 236, 233, 255 ) );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxVERTICAL );
	
	m_Eco2CheckBox = new wxCheckBox( m_Eco2Panel, ID_ECO1CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer30->Add( m_Eco2CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_Eco2Panel->SetSizer( bSizer30 );
	m_Eco2Panel->Layout();
	bSizer30->Fit( m_Eco2Panel );
	m_LayerListFlexGridSizer->Add( m_Eco2Panel, 1, wxEXPAND, 5 );
	
	m_Eco2StaticText = new wxStaticText( m_LayersListPanel, ID_ECO1CHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco2StaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco2StaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_CommentsName = new wxStaticText( m_LayersListPanel, ID_COMMENTSNAME, _("Comments_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CommentsName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_CommentsPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsPanel->SetBackgroundColour( wxColour( 236, 233, 255 ) );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	m_CommentsCheckBox = new wxCheckBox( m_CommentsPanel, ID_COMMENTSCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsCheckBox->SetToolTip( _("If you want a separate layer for comments or notes") );
	
	bSizer31->Add( m_CommentsCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_CommentsPanel->SetSizer( bSizer31 );
	m_CommentsPanel->Layout();
	bSizer31->Fit( m_CommentsPanel );
	m_LayerListFlexGridSizer->Add( m_CommentsPanel, 1, wxEXPAND, 5 );
	
	m_CommentsStaticText = new wxStaticText( m_LayersListPanel, ID_COMMENTSCHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CommentsStaticText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_DrawingsName = new wxStaticText( m_LayersListPanel, ID_DRAWINGSNAME, _("Drawings_later"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DrawingsName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_DrawingsName, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );
	
	m_DrawingsPanel = new wxPanel( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_DrawingsPanel->SetBackgroundColour( wxColour( 236, 233, 255 ) );
	
	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxVERTICAL );
	
	m_DrawingsCheckBox = new wxCheckBox( m_DrawingsPanel, ID_DRAWINGSCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DrawingsCheckBox->SetToolTip( _("If you want a layer for documentation drawings") );
	
	bSizer32->Add( m_DrawingsCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_DrawingsPanel->SetSizer( bSizer32 );
	m_DrawingsPanel->Layout();
	bSizer32->Fit( m_DrawingsPanel );
	m_LayerListFlexGridSizer->Add( m_DrawingsPanel, 1, wxEXPAND, 5 );
	
	m_DrawingsStaticText = new wxStaticText( m_LayersListPanel, ID_DRAWINGSCHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DrawingsStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_DrawingsStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	
	m_LayersListPanel->SetSizer( m_LayerListFlexGridSizer );
	m_LayersListPanel->Layout();
	m_LayerListFlexGridSizer->Fit( m_LayersListPanel );
	b_layersListSizer->Add( m_LayersListPanel, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	bMainSizer->Add( b_layersListSizer, 1, wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bMainSizer->Add( m_sdbSizer, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LAYERS_SETUP_BASE::OnSize ) );
	m_PresetsChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnPresetsChoice ), NULL, this );
	m_CopperLayersChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCopperLayersChoice ), NULL, this );
	m_CrtYdFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_FabFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_AdhesFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SoldPFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SilkSFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_MaskFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_FrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In1CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In2CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In3CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In4CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In5CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In6CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In7CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In8CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In9CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In10CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In11CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In12CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In13CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In14CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In15CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In16CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In17CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In18CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In19CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In20CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In21CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In22CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In23CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In24CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In25CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In26CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In27CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In28CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In29CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In30CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_BackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_MaskBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SilkSBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SoldPBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_AdhesBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_FabBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_CrtYdBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_PCBEdgesCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_MarginCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_Eco1CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_Eco2CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_CommentsCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_DrawingsCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnOkButtonClick ), NULL, this );
}

DIALOG_LAYERS_SETUP_BASE::~DIALOG_LAYERS_SETUP_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_LAYERS_SETUP_BASE::OnSize ) );
	m_PresetsChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnPresetsChoice ), NULL, this );
	m_CopperLayersChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCopperLayersChoice ), NULL, this );
	m_CrtYdFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_FabFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_AdhesFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SoldPFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SilkSFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_MaskFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_FrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In1CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In2CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In3CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In4CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In5CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In6CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In7CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In8CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In9CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In10CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In11CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In12CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In13CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In14CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In15CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In16CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In17CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In18CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In19CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In20CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In21CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In22CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In23CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In24CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In25CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In26CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In27CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In28CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In29CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_In30CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_BackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::DenyChangeCheckBox ), NULL, this );
	m_MaskBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SilkSBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_SoldPBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_AdhesBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_FabBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_CrtYdBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_PCBEdgesCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_MarginCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_Eco1CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_Eco2CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_CommentsCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_DrawingsCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCheckBox ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnCancelButtonClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_LAYERS_SETUP_BASE::OnOkButtonClick ), NULL, this );
	
}
