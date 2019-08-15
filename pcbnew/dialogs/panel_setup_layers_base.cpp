///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_layers_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_LAYERS_BASE::PANEL_SETUP_LAYERS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerMargins;
	bSizerMargins = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerLayerCnt;
	bSizerLayerCnt = new wxBoxSizer( wxHORIZONTAL );

	wxString m_PresetsChoiceChoices[] = { _("Custom layer set"), _("Two layers, parts on Front"), _("Two layers, parts on Back"), _("Two layers, parts on Front & Back"), _("Four layers, parts on Front"), _("Four layers, parts on Front & Back"), _("All layers on") };
	int m_PresetsChoiceNChoices = sizeof( m_PresetsChoiceChoices ) / sizeof( wxString );
	m_PresetsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PresetsChoiceNChoices, m_PresetsChoiceChoices, 0 );
	m_PresetsChoice->SetSelection( 0 );
	bSizerLayerCnt->Add( m_PresetsChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerLayerCnt->Add( 15, 0, 0, 0, 5 );

	m_staticTextCopperLayers = new wxStaticText( this, wxID_ANY, _("Copper layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextCopperLayers->Wrap( -1 );
	bSizerLayerCnt->Add( m_staticTextCopperLayers, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	wxString m_CopperLayersChoiceChoices[] = { _("2"), _("4"), _("6"), _("8"), _("10"), _("12"), _("14"), _("16"), _("18"), _("20"), _("22"), _("24"), _("26"), _("28"), _("30"), _("32") };
	int m_CopperLayersChoiceNChoices = sizeof( m_CopperLayersChoiceChoices ) / sizeof( wxString );
	m_CopperLayersChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_CopperLayersChoiceNChoices, m_CopperLayersChoiceChoices, 0 );
	m_CopperLayersChoice->SetSelection( 0 );
	bSizerLayerCnt->Add( m_CopperLayersChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 3 );


	bSizerLayerCnt->Add( 15, 0, 0, 0, 5 );

	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("PCB thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	bSizerLayerCnt->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );

	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerLayerCnt->Add( m_thicknessCtrl, 0, wxEXPAND|wxALL, 3 );

	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	bSizerLayerCnt->Add( m_thicknessUnits, 0, wxALL|wxALIGN_CENTER_VERTICAL, 3 );


	bSizerMargins->Add( bSizerLayerCnt, 0, wxEXPAND, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMargins->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_LayersListPanel = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL|wxVSCROLL );
	m_LayersListPanel->SetScrollRate( 0, 5 );
	m_LayerListFlexGridSizer = new wxFlexGridSizer( 0, 3, 2, 8 );
	m_LayerListFlexGridSizer->AddGrowableCol( 1 );
	m_LayerListFlexGridSizer->AddGrowableCol( 2 );
	m_LayerListFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_LayerListFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_CrtYdFrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_CRTYDFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	m_CrtYdFrontCheckBox->SetToolTip( _("If you want a courtyard layer for the front side of the board") );

	m_LayerListFlexGridSizer->Add( m_CrtYdFrontCheckBox, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_CENTER_HORIZONTAL|wxLEFT, 5 );

	m_CrtYdFrontName = new wxStaticText( m_LayersListPanel, ID_CRTYDFRONTNAME, _("CrtYd_Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdFrontName->Wrap( -1 );
	m_CrtYdFrontName->SetMinSize( wxSize( 110,-1 ) );

	m_LayerListFlexGridSizer->Add( m_CrtYdFrontName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_CrtYdFrontStaticText = new wxStaticText( m_LayersListPanel, ID_CRTYDFRONTCHOICE, _("Off-board, testing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdFrontStaticText->Wrap( -1 );
	m_CrtYdFrontStaticText->SetMinSize( wxSize( 150,-1 ) );

	m_LayerListFlexGridSizer->Add( m_CrtYdFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_FabFrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_FABFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontCheckBox->SetToolTip( _("If you want a fabrication layer for the front side of the board") );

	m_LayerListFlexGridSizer->Add( m_FabFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_FabFrontName = new wxStaticText( m_LayersListPanel, ID_FABFRONTNAME, _("Fab_Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabFrontName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_FabFrontStaticText = new wxStaticText( m_LayersListPanel, ID_FABFRONTCHOICE, _("Off-board, manufacturing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_AdhesFrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_ADHESFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontCheckBox->SetToolTip( _("If you want an adhesive template for the front side of the board") );

	m_LayerListFlexGridSizer->Add( m_AdhesFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_AdhesFrontName = new wxStaticText( m_LayersListPanel, ID_ADHESFRONTNAME, _("Adhes_Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesFrontName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_AdhesFrontStaticText = new wxStaticText( m_LayersListPanel, ID_ADHESFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SoldPFrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_SOLDPFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontCheckBox->SetToolTip( _("If you want a solder paster layer for front side of the board") );

	m_LayerListFlexGridSizer->Add( m_SoldPFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SoldPFrontName = new wxStaticText( m_LayersListPanel, ID_SOLDPFRONTNAME, _("SoldP_Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPFrontName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SoldPFrontStaticText = new wxStaticText( m_LayersListPanel, ID_SOLDPFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SilkSFrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_SILKSFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSFrontCheckBox->SetToolTip( _("If you want a silk screen layer for the front side of the board") );

	m_LayerListFlexGridSizer->Add( m_SilkSFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SilkSFrontName = new wxStaticText( m_LayersListPanel, ID_SILKSFRONTNAME, _("SilkS_Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSFrontName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SilkSFrontStaticText = new wxStaticText( m_LayersListPanel, ID_SILKSFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_MaskFrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_MASKFRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskFrontCheckBox->SetToolTip( _("If you want a solder mask layer for the front of the board") );

	m_LayerListFlexGridSizer->Add( m_MaskFrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_MaskFrontName = new wxStaticText( m_LayersListPanel, ID_MASKFRONTNAME, _("Mask_Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskFrontName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskFrontName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_MaskFrontStaticText = new wxStaticText( m_LayersListPanel, ID_MASKFRONTCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskFrontStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskFrontStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_FrontCheckBox = new wxCheckBox( m_LayersListPanel, ID_FRONTCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontCheckBox->SetToolTip( _("If you want a front copper layer") );

	m_LayerListFlexGridSizer->Add( m_FrontCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_FrontName = new wxTextCtrl( m_LayersListPanel, ID_FRONTNAME, _("Front_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontName->SetToolTip( _("Layer name of front (top) copper layer") );

	m_LayerListFlexGridSizer->Add( m_FrontName, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_FrontChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_FrontChoiceNChoices = sizeof( m_FrontChoiceChoices ) / sizeof( wxString );
	m_FrontChoice = new wxChoice( m_LayersListPanel, ID_FRONTCHOICE, wxDefaultPosition, wxDefaultSize, m_FrontChoiceNChoices, m_FrontChoiceChoices, 0 );
	m_FrontChoice->SetSelection( 0 );
	m_FrontChoice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_FrontChoice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In1CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN1CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In1CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In1Name = new wxTextCtrl( m_LayersListPanel, ID_IN1NAME, _("In1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In1Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In1ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In1ChoiceNChoices = sizeof( m_In1ChoiceChoices ) / sizeof( wxString );
	m_In1Choice = new wxChoice( m_LayersListPanel, ID_IN1CHOICE, wxDefaultPosition, wxDefaultSize, m_In1ChoiceNChoices, m_In1ChoiceChoices, 0 );
	m_In1Choice->SetSelection( 0 );
	m_In1Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In1Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In2CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN2CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In2CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In2Name = new wxTextCtrl( m_LayersListPanel, ID_IN2NAME, _("In2"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In2Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In2ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In2ChoiceNChoices = sizeof( m_In2ChoiceChoices ) / sizeof( wxString );
	m_In2Choice = new wxChoice( m_LayersListPanel, ID_IN2CHOICE, wxDefaultPosition, wxDefaultSize, m_In2ChoiceNChoices, m_In2ChoiceChoices, 0 );
	m_In2Choice->SetSelection( 0 );
	m_In2Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In2Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In3CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN3CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In3CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In3Name = new wxTextCtrl( m_LayersListPanel, ID_IN3NAME, _("In3"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In3Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In3ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In3ChoiceNChoices = sizeof( m_In3ChoiceChoices ) / sizeof( wxString );
	m_In3Choice = new wxChoice( m_LayersListPanel, ID_IN3CHOICE, wxDefaultPosition, wxDefaultSize, m_In3ChoiceNChoices, m_In3ChoiceChoices, 0 );
	m_In3Choice->SetSelection( 0 );
	m_In3Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In3Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In4CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN4CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In4CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In4Name = new wxTextCtrl( m_LayersListPanel, ID_IN4NAME, _("In4"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In4Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In4ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In4ChoiceNChoices = sizeof( m_In4ChoiceChoices ) / sizeof( wxString );
	m_In4Choice = new wxChoice( m_LayersListPanel, ID_IN4CHOICE, wxDefaultPosition, wxDefaultSize, m_In4ChoiceNChoices, m_In4ChoiceChoices, 0 );
	m_In4Choice->SetSelection( 0 );
	m_In4Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In4Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In5CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN5CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In5CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In5Name = new wxTextCtrl( m_LayersListPanel, ID_IN5NAME, _("In5"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In5Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In5ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In5ChoiceNChoices = sizeof( m_In5ChoiceChoices ) / sizeof( wxString );
	m_In5Choice = new wxChoice( m_LayersListPanel, ID_IN5CHOICE, wxDefaultPosition, wxDefaultSize, m_In5ChoiceNChoices, m_In5ChoiceChoices, 0 );
	m_In5Choice->SetSelection( 0 );
	m_In5Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In5Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In6CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN6CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In6CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In6Name = new wxTextCtrl( m_LayersListPanel, ID_IN6NAME, _("In6"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In6Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In6ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In6ChoiceNChoices = sizeof( m_In6ChoiceChoices ) / sizeof( wxString );
	m_In6Choice = new wxChoice( m_LayersListPanel, ID_IN6CHOICE, wxDefaultPosition, wxDefaultSize, m_In6ChoiceNChoices, m_In6ChoiceChoices, 0 );
	m_In6Choice->SetSelection( 0 );
	m_In6Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In6Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In7CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN7CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In7CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In7Name = new wxTextCtrl( m_LayersListPanel, ID_IN7NAME, _("In7"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In7Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In7ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In7ChoiceNChoices = sizeof( m_In7ChoiceChoices ) / sizeof( wxString );
	m_In7Choice = new wxChoice( m_LayersListPanel, ID_IN7CHOICE, wxDefaultPosition, wxDefaultSize, m_In7ChoiceNChoices, m_In7ChoiceChoices, 0 );
	m_In7Choice->SetSelection( 0 );
	m_In7Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In7Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In8CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN8CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In8CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In8Name = new wxTextCtrl( m_LayersListPanel, ID_IN8NAME, _("In8"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In8Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In8ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In8ChoiceNChoices = sizeof( m_In8ChoiceChoices ) / sizeof( wxString );
	m_In8Choice = new wxChoice( m_LayersListPanel, ID_IN8CHOICE, wxDefaultPosition, wxDefaultSize, m_In8ChoiceNChoices, m_In8ChoiceChoices, 0 );
	m_In8Choice->SetSelection( 0 );
	m_In8Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In8Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In9CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN9CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In9CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In9Name = new wxTextCtrl( m_LayersListPanel, ID_IN9NAME, _("In9"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In9Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In9ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In9ChoiceNChoices = sizeof( m_In9ChoiceChoices ) / sizeof( wxString );
	m_In9Choice = new wxChoice( m_LayersListPanel, ID_IN9CHOICE, wxDefaultPosition, wxDefaultSize, m_In9ChoiceNChoices, m_In9ChoiceChoices, 0 );
	m_In9Choice->SetSelection( 0 );
	m_In9Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In9Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In10CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN10CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In10CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In10Name = new wxTextCtrl( m_LayersListPanel, ID_IN10NAME, _("In10"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In10Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In10ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In10ChoiceNChoices = sizeof( m_In10ChoiceChoices ) / sizeof( wxString );
	m_In10Choice = new wxChoice( m_LayersListPanel, ID_IN10CHOICE, wxDefaultPosition, wxDefaultSize, m_In10ChoiceNChoices, m_In10ChoiceChoices, 0 );
	m_In10Choice->SetSelection( 0 );
	m_In10Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In10Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In11CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN11CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In11CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In11Name = new wxTextCtrl( m_LayersListPanel, ID_IN11NAME, _("In11"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In11Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In11ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In11ChoiceNChoices = sizeof( m_In11ChoiceChoices ) / sizeof( wxString );
	m_In11Choice = new wxChoice( m_LayersListPanel, ID_IN11CHOICE, wxDefaultPosition, wxDefaultSize, m_In11ChoiceNChoices, m_In11ChoiceChoices, 0 );
	m_In11Choice->SetSelection( 0 );
	m_In11Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In11Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In12CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN12CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In12CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In12Name = new wxTextCtrl( m_LayersListPanel, ID_IN12NAME, _("In12"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In12Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In12ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In12ChoiceNChoices = sizeof( m_In12ChoiceChoices ) / sizeof( wxString );
	m_In12Choice = new wxChoice( m_LayersListPanel, ID_IN12CHOICE, wxDefaultPosition, wxDefaultSize, m_In12ChoiceNChoices, m_In12ChoiceChoices, 0 );
	m_In12Choice->SetSelection( 0 );
	m_In12Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In12Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In13CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN13CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In13CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In13Name = new wxTextCtrl( m_LayersListPanel, ID_IN13NAME, _("In13"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In13Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In13ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In13ChoiceNChoices = sizeof( m_In13ChoiceChoices ) / sizeof( wxString );
	m_In13Choice = new wxChoice( m_LayersListPanel, ID_IN13CHOICE, wxDefaultPosition, wxDefaultSize, m_In13ChoiceNChoices, m_In13ChoiceChoices, 0 );
	m_In13Choice->SetSelection( 0 );
	m_In13Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In13Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In14CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN14CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In14CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In14Name = new wxTextCtrl( m_LayersListPanel, ID_IN14NAME, _("In14"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In14Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In14ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In14ChoiceNChoices = sizeof( m_In14ChoiceChoices ) / sizeof( wxString );
	m_In14Choice = new wxChoice( m_LayersListPanel, ID_IN14CHOICE, wxDefaultPosition, wxDefaultSize, m_In14ChoiceNChoices, m_In14ChoiceChoices, 0 );
	m_In14Choice->SetSelection( 0 );
	m_In14Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In14Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In15CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN15CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In15CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In15Name = new wxTextCtrl( m_LayersListPanel, ID_IN15NAME, _("In15"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In15Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In15ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In15ChoiceNChoices = sizeof( m_In15ChoiceChoices ) / sizeof( wxString );
	m_In15Choice = new wxChoice( m_LayersListPanel, ID_IN15CHOICE, wxDefaultPosition, wxDefaultSize, m_In15ChoiceNChoices, m_In15ChoiceChoices, 0 );
	m_In15Choice->SetSelection( 0 );
	m_In15Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In15Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In16CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN16CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In16CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In16Name = new wxTextCtrl( m_LayersListPanel, ID_IN16NAME, _("In16"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In16Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In16ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In16ChoiceNChoices = sizeof( m_In16ChoiceChoices ) / sizeof( wxString );
	m_In16Choice = new wxChoice( m_LayersListPanel, ID_IN16CHOICE, wxDefaultPosition, wxDefaultSize, m_In16ChoiceNChoices, m_In16ChoiceChoices, 0 );
	m_In16Choice->SetSelection( 0 );
	m_In16Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In16Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In17CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN17CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In17CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In17Name = new wxTextCtrl( m_LayersListPanel, ID_IN17NAME, _("In17"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In17Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In17ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In17ChoiceNChoices = sizeof( m_In17ChoiceChoices ) / sizeof( wxString );
	m_In17Choice = new wxChoice( m_LayersListPanel, ID_IN17CHOICE, wxDefaultPosition, wxDefaultSize, m_In17ChoiceNChoices, m_In17ChoiceChoices, 0 );
	m_In17Choice->SetSelection( 0 );
	m_In17Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In17Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In18CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN18CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In18CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In18Name = new wxTextCtrl( m_LayersListPanel, ID_IN18NAME, _("In18"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In18Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In18ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In18ChoiceNChoices = sizeof( m_In18ChoiceChoices ) / sizeof( wxString );
	m_In18Choice = new wxChoice( m_LayersListPanel, ID_IN18CHOICE, wxDefaultPosition, wxDefaultSize, m_In18ChoiceNChoices, m_In18ChoiceChoices, 0 );
	m_In18Choice->SetSelection( 0 );
	m_In18Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In18Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In19CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN19CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In19CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In19Name = new wxTextCtrl( m_LayersListPanel, ID_IN19NAME, _("In19"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In19Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In19ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In19ChoiceNChoices = sizeof( m_In19ChoiceChoices ) / sizeof( wxString );
	m_In19Choice = new wxChoice( m_LayersListPanel, ID_IN19CHOICE, wxDefaultPosition, wxDefaultSize, m_In19ChoiceNChoices, m_In19ChoiceChoices, 0 );
	m_In19Choice->SetSelection( 0 );
	m_In19Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In19Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In20CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN20CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In20CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In20Name = new wxTextCtrl( m_LayersListPanel, ID_IN20NAME, _("In20"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In20Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In20ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In20ChoiceNChoices = sizeof( m_In20ChoiceChoices ) / sizeof( wxString );
	m_In20Choice = new wxChoice( m_LayersListPanel, ID_IN20CHOICE, wxDefaultPosition, wxDefaultSize, m_In20ChoiceNChoices, m_In20ChoiceChoices, 0 );
	m_In20Choice->SetSelection( 0 );
	m_In20Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In20Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In21CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN21CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In21CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In21Name = new wxTextCtrl( m_LayersListPanel, ID_IN21NAME, _("In21"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In21Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In21ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In21ChoiceNChoices = sizeof( m_In21ChoiceChoices ) / sizeof( wxString );
	m_In21Choice = new wxChoice( m_LayersListPanel, ID_IN21CHOICE, wxDefaultPosition, wxDefaultSize, m_In21ChoiceNChoices, m_In21ChoiceChoices, 0 );
	m_In21Choice->SetSelection( 0 );
	m_In21Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In21Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In22CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN22CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In22CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In22Name = new wxTextCtrl( m_LayersListPanel, ID_IN22NAME, _("In22"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In22Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In22ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In22ChoiceNChoices = sizeof( m_In22ChoiceChoices ) / sizeof( wxString );
	m_In22Choice = new wxChoice( m_LayersListPanel, ID_IN22CHOICE, wxDefaultPosition, wxDefaultSize, m_In22ChoiceNChoices, m_In22ChoiceChoices, 0 );
	m_In22Choice->SetSelection( 0 );
	m_In22Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In22Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In23CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN23CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In23CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In23Name = new wxTextCtrl( m_LayersListPanel, ID_IN23NAME, _("In23"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In23Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In23ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In23ChoiceNChoices = sizeof( m_In23ChoiceChoices ) / sizeof( wxString );
	m_In23Choice = new wxChoice( m_LayersListPanel, ID_IN22CHOICE, wxDefaultPosition, wxDefaultSize, m_In23ChoiceNChoices, m_In23ChoiceChoices, 0 );
	m_In23Choice->SetSelection( 0 );
	m_In23Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In23Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In24CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN24CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In24CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In24Name = new wxTextCtrl( m_LayersListPanel, ID_IN24NAME, _("In24"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In24Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In24ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In24ChoiceNChoices = sizeof( m_In24ChoiceChoices ) / sizeof( wxString );
	m_In24Choice = new wxChoice( m_LayersListPanel, ID_IN24CHOICE, wxDefaultPosition, wxDefaultSize, m_In24ChoiceNChoices, m_In24ChoiceChoices, 0 );
	m_In24Choice->SetSelection( 0 );
	m_In24Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In24Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In25CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN25CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In25CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In25Name = new wxTextCtrl( m_LayersListPanel, ID_IN25NAME, _("In25"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In25Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In25ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In25ChoiceNChoices = sizeof( m_In25ChoiceChoices ) / sizeof( wxString );
	m_In25Choice = new wxChoice( m_LayersListPanel, ID_IN25CHOICE, wxDefaultPosition, wxDefaultSize, m_In25ChoiceNChoices, m_In25ChoiceChoices, 0 );
	m_In25Choice->SetSelection( 0 );
	m_In25Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In25Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In26CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN26CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In26CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In26Name = new wxTextCtrl( m_LayersListPanel, ID_IN26NAME, _("In26"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In26Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In26ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In26ChoiceNChoices = sizeof( m_In26ChoiceChoices ) / sizeof( wxString );
	m_In26Choice = new wxChoice( m_LayersListPanel, ID_IN26CHOICE, wxDefaultPosition, wxDefaultSize, m_In26ChoiceNChoices, m_In26ChoiceChoices, 0 );
	m_In26Choice->SetSelection( 0 );
	m_In26Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In26Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In27CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN27CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In27CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In27Name = new wxTextCtrl( m_LayersListPanel, ID_IN27NAME, _("In27"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In27Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In27ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In27ChoiceNChoices = sizeof( m_In27ChoiceChoices ) / sizeof( wxString );
	m_In27Choice = new wxChoice( m_LayersListPanel, ID_IN27CHOICE, wxDefaultPosition, wxDefaultSize, m_In27ChoiceNChoices, m_In27ChoiceChoices, 0 );
	m_In27Choice->SetSelection( 0 );
	m_In27Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In27Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In28CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN28CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In28CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In28Name = new wxTextCtrl( m_LayersListPanel, ID_IN28NAME, _("In28"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In28Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In28ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In28ChoiceNChoices = sizeof( m_In28ChoiceChoices ) / sizeof( wxString );
	m_In28Choice = new wxChoice( m_LayersListPanel, ID_IN28CHOICE, wxDefaultPosition, wxDefaultSize, m_In28ChoiceNChoices, m_In28ChoiceChoices, 0 );
	m_In28Choice->SetSelection( 0 );
	m_In28Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In28Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In29CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN29CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In29CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In29Name = new wxTextCtrl( m_LayersListPanel, ID_IN29NAME, _("In29"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In29Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In29ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In29ChoiceNChoices = sizeof( m_In29ChoiceChoices ) / sizeof( wxString );
	m_In29Choice = new wxChoice( m_LayersListPanel, ID_IN29CHOICE, wxDefaultPosition, wxDefaultSize, m_In29ChoiceNChoices, m_In29ChoiceChoices, 0 );
	m_In29Choice->SetSelection( 0 );
	m_In29Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In29Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_In30CheckBox = new wxCheckBox( m_LayersListPanel, ID_IN30CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In30CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_In30Name = new wxTextCtrl( m_LayersListPanel, ID_IN30NAME, _("In30"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_In30Name, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_In30ChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_In30ChoiceNChoices = sizeof( m_In30ChoiceChoices ) / sizeof( wxString );
	m_In30Choice = new wxChoice( m_LayersListPanel, ID_IN30CHOICE, wxDefaultPosition, wxDefaultSize, m_In30ChoiceNChoices, m_In30ChoiceChoices, 0 );
	m_In30Choice->SetSelection( 0 );
	m_In30Choice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_In30Choice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_BackCheckBox = new wxCheckBox( m_LayersListPanel, ID_BACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_BackCheckBox->SetToolTip( _("If you want a back copper layer") );

	m_LayerListFlexGridSizer->Add( m_BackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_BackName = new wxTextCtrl( m_LayersListPanel, ID_BACKNAME, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BackName->SetToolTip( _("Layer name of back (bottom) copper layer") );

	m_LayerListFlexGridSizer->Add( m_BackName, 0, wxEXPAND|wxRIGHT, 5 );

	wxString m_BackChoiceChoices[] = { _("signal"), _("power plane"), _("mixed"), _("jumper") };
	int m_BackChoiceNChoices = sizeof( m_BackChoiceChoices ) / sizeof( wxString );
	m_BackChoice = new wxChoice( m_LayersListPanel, ID_BACKCHOICE, wxDefaultPosition, wxDefaultSize, m_BackChoiceNChoices, m_BackChoiceChoices, 0 );
	m_BackChoice->SetSelection( 0 );
	m_BackChoice->SetToolTip( _("Copper layer type for Freerouter and other external routers.\nPower plane layers are removed from Freerouter's layer menus.") );

	m_LayerListFlexGridSizer->Add( m_BackChoice, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_MaskBackCheckBox = new wxCheckBox( m_LayersListPanel, ID_MASKBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackCheckBox->SetToolTip( _("If you want a solder mask layer for the back side of the board") );

	m_LayerListFlexGridSizer->Add( m_MaskBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_MaskBackName = new wxStaticText( m_LayersListPanel, ID_MASKBACKNAME, _("Mask_Back_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskBackName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_MaskBackStaticText = new wxStaticText( m_LayersListPanel, ID_MASKBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MaskBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SilkSBackCheckBox = new wxCheckBox( m_LayersListPanel, ID_SILKSBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackCheckBox->SetToolTip( _("If you want a silk screen layer for the back side of the board") );

	m_LayerListFlexGridSizer->Add( m_SilkSBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SilkSBackName = new wxStaticText( m_LayersListPanel, ID_SILKSBACKNAME, _("SilkS_Back_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSBackName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SilkSBackStaticText = new wxStaticText( m_LayersListPanel, ID_SILKSBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SilkSBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SilkSBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SoldPBackCheckBox = new wxCheckBox( m_LayersListPanel, ID_SOLDPBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackCheckBox->SetToolTip( _("If you want a solder paste layer for the back side of the board") );

	m_LayerListFlexGridSizer->Add( m_SoldPBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_SoldPBackName = new wxStaticText( m_LayersListPanel, ID_SOLDPBACKNAME, _("SoldP_Back_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPBackName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SoldPBackStaticText = new wxStaticText( m_LayersListPanel, ID_SOLDPBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SoldPBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_SoldPBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_AdhesBackCheckBox = new wxCheckBox( m_LayersListPanel, ID_ADHESBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackCheckBox->SetToolTip( _("If you want an adhesive layer for the back side of the board") );

	m_LayerListFlexGridSizer->Add( m_AdhesBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_AdhesBackName = new wxStaticText( m_LayersListPanel, ID_ADHESBACKNAME, _("Adhes_Back_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesBackName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_AdhesBackStaticText = new wxStaticText( m_LayersListPanel, ID_ADHESBACKCHOICE, _("On-board, non-copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AdhesBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_AdhesBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_FabBackCheckBox = new wxCheckBox( m_LayersListPanel, ID_FABBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackCheckBox->SetToolTip( _("If you want a fabrication layer for the back side of the board") );

	m_LayerListFlexGridSizer->Add( m_FabBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_FabBackName = new wxStaticText( m_LayersListPanel, ID_FABBACKNAME, _("Fab_Back_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabBackName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_FabBackStaticText = new wxStaticText( m_LayersListPanel, ID_FABBACKCHOICE, _("Off-board, manufacturing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FabBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_FabBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_CrtYdBackCheckBox = new wxCheckBox( m_LayersListPanel, ID_CRTYDBACKCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackCheckBox->SetToolTip( _("If you want a courtyard layer for the back side of the board") );

	m_LayerListFlexGridSizer->Add( m_CrtYdBackCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_CrtYdBackName = new wxStaticText( m_LayersListPanel, ID_CRTYDBACKNAME, _("CrtYd_Back_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CrtYdBackName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_CrtYdBackStaticText = new wxStaticText( m_LayersListPanel, ID_CRTYDBACKCHOICE, _("Off-board, testing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CrtYdBackStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CrtYdBackStaticText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_PCBEdgesCheckBox = new wxCheckBox( m_LayersListPanel, ID_PCBEDGESCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesCheckBox->SetToolTip( _("If you want a board perimeter layer") );

	m_LayerListFlexGridSizer->Add( m_PCBEdgesCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_PCBEdgesName = new wxStaticText( m_LayersListPanel, ID_PCBEDGESNAME, _("PCB_Edges_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_PCBEdgesName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_PCBEdgesStaticText = new wxStaticText( m_LayersListPanel, ID_PCBEDGESCHOICE, _("Board contour"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PCBEdgesStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_PCBEdgesStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_MarginCheckBox = new wxCheckBox( m_LayersListPanel, ID_MARGINCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_MarginCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_MarginName = new wxStaticText( m_LayersListPanel, ID_MARGINNAME, _("Margin_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MarginName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MarginName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_MarginStaticText = new wxStaticText( m_LayersListPanel, ID_ECO2CHOICE, _("Edge_Cuts setback"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MarginStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_MarginStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_Eco1CheckBox = new wxCheckBox( m_LayersListPanel, ID_ECO2CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_Eco1CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Eco1Name = new wxStaticText( m_LayersListPanel, ID_ECO2NAME, _("Eco1_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco1Name->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco1Name, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Eco1StaticText = new wxStaticText( m_LayersListPanel, ID_ECO2CHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco1StaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco1StaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_Eco2CheckBox = new wxCheckBox( m_LayersListPanel, ID_ECO1CHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerListFlexGridSizer->Add( m_Eco2CheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_Eco2Name = new wxStaticText( m_LayersListPanel, ID_ECO1NAME, _("Eco2_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco2Name->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco2Name, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_Eco2StaticText = new wxStaticText( m_LayersListPanel, ID_ECO1CHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Eco2StaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_Eco2StaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_CommentsCheckBox = new wxCheckBox( m_LayersListPanel, ID_COMMENTSCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsCheckBox->SetToolTip( _("If you want a separate layer for comments or notes") );

	m_LayerListFlexGridSizer->Add( m_CommentsCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_CommentsName = new wxStaticText( m_LayersListPanel, ID_COMMENTSNAME, _("Comments_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CommentsName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_CommentsStaticText = new wxStaticText( m_LayersListPanel, ID_COMMENTSCHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_CommentsStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_CommentsStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_DrawingsCheckBox = new wxCheckBox( m_LayersListPanel, ID_DRAWINGSCHECKBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DrawingsCheckBox->SetToolTip( _("If you want a layer for documentation drawings") );

	m_LayerListFlexGridSizer->Add( m_DrawingsCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_DrawingsName = new wxStaticText( m_LayersListPanel, ID_DRAWINGSNAME, _("Drawings_layer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DrawingsName->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_DrawingsName, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_DrawingsStaticText = new wxStaticText( m_LayersListPanel, ID_DRAWINGSCHOICE, _("Auxiliary"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DrawingsStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_DrawingsStaticText, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	m_LayersListPanel->SetSizer( m_LayerListFlexGridSizer );
	m_LayersListPanel->Layout();
	m_LayerListFlexGridSizer->Fit( m_LayersListPanel );
	bSizerMargins->Add( m_LayersListPanel, 1, wxEXPAND|wxALL, 5 );


	bMainSizer->Add( bSizerMargins, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	this->SetSizer( bMainSizer );
	this->Layout();

	// Connect Events
	m_PresetsChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnPresetsChoice ), NULL, this );
	m_CopperLayersChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCopperLayersChoice ), NULL, this );
	m_CrtYdFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_FabFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_AdhesFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SoldPFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SilkSFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_MaskFrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_FrontCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In1CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In2CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In3CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In4CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In5CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In6CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In7CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In8CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In9CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In10CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In11CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In12CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In13CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In14CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In15CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In16CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In17CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In18CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In19CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In20CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In21CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In22CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In23CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In24CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In25CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In26CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In27CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In28CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In29CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In30CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_BackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_MaskBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SilkSBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SoldPBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_AdhesBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_FabBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_CrtYdBackCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_PCBEdgesCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_MarginCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_Eco1CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_Eco2CheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_CommentsCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_DrawingsCheckBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
}

PANEL_SETUP_LAYERS_BASE::~PANEL_SETUP_LAYERS_BASE()
{
	// Disconnect Events
	m_PresetsChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnPresetsChoice ), NULL, this );
	m_CopperLayersChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCopperLayersChoice ), NULL, this );
	m_CrtYdFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_FabFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_AdhesFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SoldPFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SilkSFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_MaskFrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_FrontCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In1CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In2CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In3CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In4CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In5CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In6CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In7CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In8CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In9CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In10CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In11CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In12CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In13CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In14CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In15CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In16CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In17CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In18CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In19CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In20CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In21CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In22CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In23CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In24CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In25CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In26CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In27CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In28CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In29CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_In30CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_BackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_MaskBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SilkSBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_SoldPBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_AdhesBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_FabBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_CrtYdBackCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_PCBEdgesCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_MarginCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::DenyChangeCheckBox ), NULL, this );
	m_Eco1CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_Eco2CheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_CommentsCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );
	m_DrawingsCheckBox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( PANEL_SETUP_LAYERS_BASE::OnCheckBox ), NULL, this );

}
