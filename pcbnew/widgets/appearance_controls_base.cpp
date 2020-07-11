///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "appearance_controls_base.h"

///////////////////////////////////////////////////////////////////////////

APPEARANCE_CONTROLS_BASE::APPEARANCE_CONTROLS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	this->SetFont( wxFont( 9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_sizerOuter = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelLayers = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLayersSizer = new wxBoxSizer( wxVERTICAL );

	m_windowLayers = new wxScrolledWindow( m_panelLayers, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_windowLayers->SetScrollRate( 0, 5 );
	m_windowLayers->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_layers_outer_sizer = new wxBoxSizer( wxVERTICAL );


	m_windowLayers->SetSizer( m_layers_outer_sizer );
	m_windowLayers->Layout();
	m_layers_outer_sizer->Fit( m_windowLayers );
	m_panelLayersSizer->Add( m_windowLayers, 1, wxEXPAND | wxALL, 5 );

	m_paneLayerDisplay = new wxCollapsiblePane( m_panelLayers, wxID_ANY, wxT("Layer Display Options"), wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE|wxCP_NO_TLW_RESIZE|wxBORDER_NONE|wxCLIP_CHILDREN|wxFULL_REPAINT_ON_RESIZE );
	m_paneLayerDisplay->Collapse( true );

	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxVERTICAL );

	m_staticText13 = new wxStaticText( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Show non-active layers as:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	bSizer121->Add( m_staticText13, 0, wxALL, 5 );

	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxHORIZONTAL );

	m_rbHighContrastNormal = new wxRadioButton( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Normal"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbHighContrastNormal->SetValue( true );
	m_rbHighContrastNormal->SetToolTip( wxT("Non-active layers will be shown in full color") );

	bSizer19->Add( m_rbHighContrastNormal, 0, wxALL, 5 );

	m_rbHighContrastDim = new wxRadioButton( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Dimmed"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbHighContrastDim->SetToolTip( wxT("Non-active layers will be dimmed") );

	bSizer19->Add( m_rbHighContrastDim, 0, wxALL, 5 );

	m_rbHighContrastOff = new wxRadioButton( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Off"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbHighContrastOff->SetToolTip( wxT("Non-active layers will be hidden") );

	bSizer19->Add( m_rbHighContrastOff, 0, wxALL, 5 );


	bSizer121->Add( bSizer19, 0, wxEXPAND, 5 );

	m_staticline5 = new wxStaticLine( m_paneLayerDisplay->GetPane(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer121->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );

	m_cbFlipBoard = new wxCheckBox( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Flip board view"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer121->Add( m_cbFlipBoard, 0, wxALL, 5 );


	m_paneLayerDisplay->GetPane()->SetSizer( bSizer121 );
	m_paneLayerDisplay->GetPane()->Layout();
	bSizer121->Fit( m_paneLayerDisplay->GetPane() );
	m_panelLayersSizer->Add( m_paneLayerDisplay, 0, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	m_panelLayers->SetSizer( m_panelLayersSizer );
	m_panelLayers->Layout();
	m_panelLayersSizer->Fit( m_panelLayers );
	m_notebook->AddPage( m_panelLayers, wxT("Layers"), true );
	m_panelObjects = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelObjects->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_objectsPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_windowObjects = new wxScrolledWindow( m_panelObjects, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_windowObjects->SetScrollRate( 0, 5 );
	m_windowObjects->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_objectsSizer = new wxGridBagSizer( 0, 0 );
	m_objectsSizer->SetFlexibleDirection( wxBOTH );
	m_objectsSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	m_windowObjects->SetSizer( m_objectsSizer );
	m_windowObjects->Layout();
	m_objectsSizer->Fit( m_windowObjects );
	m_objectsPanelSizer->Add( m_windowObjects, 1, wxEXPAND | wxALL, 5 );


	m_panelObjects->SetSizer( m_objectsPanelSizer );
	m_panelObjects->Layout();
	m_objectsPanelSizer->Fit( m_panelObjects );
	m_notebook->AddPage( m_panelObjects, wxT("Objects"), false );
	m_panelNetsAndClasses = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );

	m_netsTabSplitter = new wxSplitterWindow( m_panelNetsAndClasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_netsTabSplitter->SetSashGravity( 0.8 );
	m_netsTabSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( APPEARANCE_CONTROLS_BASE::m_netsTabSplitterOnIdle ), NULL, this );
	m_netsTabSplitter->SetMinimumPaneSize( 80 );

	m_panelNets = new wxPanel( m_netsTabSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer192;
	bSizer192 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText141 = new wxStaticText( m_panelNets, wxID_ANY, wxT("Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText141->Wrap( -1 );
	m_staticText141->SetFont( wxFont( 9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer17->Add( m_staticText141, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_txtNetFilter = new wxTextCtrl( m_panelNets, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtNetFilter->Hide();

	bSizer17->Add( m_txtNetFilter, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxTOP, 5 );

	m_btnNetInspector = new wxBitmapButton( m_panelNets, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_btnNetInspector->SetToolTip( wxT("Show the Net Inspector") );

	bSizer17->Add( m_btnNetInspector, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizer192->Add( bSizer17, 0, wxEXPAND, 5 );

	m_netsScrolledWindow = new wxScrolledWindow( m_panelNets, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_netsScrolledWindow->SetScrollRate( 5, 5 );
	m_netsOuterSizer = new wxBoxSizer( wxVERTICAL );


	m_netsScrolledWindow->SetSizer( m_netsOuterSizer );
	m_netsScrolledWindow->Layout();
	m_netsOuterSizer->Fit( m_netsScrolledWindow );
	bSizer192->Add( m_netsScrolledWindow, 1, wxEXPAND | wxALL, 5 );


	m_panelNets->SetSizer( bSizer192 );
	m_panelNets->Layout();
	bSizer192->Fit( m_panelNets );
	m_panelNetclasses = new wxPanel( m_netsTabSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText14 = new wxStaticText( m_panelNetclasses, wxID_ANY, wxT("Net Classes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_staticText14->SetFont( wxFont( 9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer20->Add( m_staticText14, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_btnConfigureNetClasses = new wxBitmapButton( m_panelNetclasses, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_btnConfigureNetClasses->SetToolTip( wxT("Configure net classes") );

	bSizer20->Add( m_btnConfigureNetClasses, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizer21->Add( bSizer20, 0, wxEXPAND, 5 );

	m_netclassScrolledWindow = new wxScrolledWindow( m_panelNetclasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_netclassScrolledWindow->SetScrollRate( 5, 5 );
	m_netclassOuterSizer = new wxBoxSizer( wxVERTICAL );


	m_netclassScrolledWindow->SetSizer( m_netclassOuterSizer );
	m_netclassScrolledWindow->Layout();
	m_netclassOuterSizer->Fit( m_netclassScrolledWindow );
	bSizer21->Add( m_netclassScrolledWindow, 1, wxEXPAND | wxALL, 5 );


	m_panelNetclasses->SetSizer( bSizer21 );
	m_panelNetclasses->Layout();
	bSizer21->Fit( m_panelNetclasses );
	m_netsTabSplitter->SplitHorizontally( m_panelNets, m_panelNetclasses, 300 );
	bSizer16->Add( m_netsTabSplitter, 1, wxEXPAND, 5 );

	m_paneNetDisplay = new wxCollapsiblePane( m_panelNetsAndClasses, wxID_ANY, wxT("Net Display Options"), wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE|wxCP_NO_TLW_RESIZE );
	m_paneNetDisplay->Collapse( true );

	wxBoxSizer* bSizer1211;
	bSizer1211 = new wxBoxSizer( wxVERTICAL );

	m_staticText131 = new wxStaticText( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("Show net colors on:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText131->Wrap( -1 );
	m_staticText131->SetToolTip( wxT("Choose when to show net and netclass colors") );

	bSizer1211->Add( m_staticText131, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer191;
	bSizer191 = new wxBoxSizer( wxHORIZONTAL );

	m_rbNetColorOff = new wxRadioButton( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("Off"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbNetColorOff->SetToolTip( wxT("Net and netclass colors are not shown") );

	bSizer191->Add( m_rbNetColorOff, 0, wxLEFT|wxTOP, 5 );

	m_rbNetColorRatsnest = new wxRadioButton( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("Ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbNetColorRatsnest->SetValue( true );
	m_rbNetColorRatsnest->SetToolTip( wxT("Net and netclass colors are shown on the ratsnest only") );

	bSizer191->Add( m_rbNetColorRatsnest, 0, wxLEFT|wxTOP, 5 );

	m_rbNetColorAll = new wxRadioButton( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("All Items"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbNetColorAll->SetToolTip( wxT("Net and netclass colors are shown on all copper items") );

	bSizer191->Add( m_rbNetColorAll, 0, wxALL, 5 );


	bSizer1211->Add( bSizer191, 0, wxEXPAND, 5 );


	m_paneNetDisplay->GetPane()->SetSizer( bSizer1211 );
	m_paneNetDisplay->GetPane()->Layout();
	bSizer1211->Fit( m_paneNetDisplay->GetPane() );
	bSizer16->Add( m_paneNetDisplay, 0, wxEXPAND | wxALL, 5 );


	m_panelNetsAndClasses->SetSizer( bSizer16 );
	m_panelNetsAndClasses->Layout();
	bSizer16->Fit( m_panelNetsAndClasses );
	m_notebook->AddPage( m_panelNetsAndClasses, wxT("Nets"), false );

	m_sizerOuter->Add( m_notebook, 1, wxEXPAND | wxALL, 5 );

	wxStaticBoxSizer* presetsSizer;
	presetsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Layer Presets") ), wxHORIZONTAL );

	wxString m_cbLayerPresetsChoices[] = { wxT("All Layers"), wxT("(unsaved)") };
	int m_cbLayerPresetsNChoices = sizeof( m_cbLayerPresetsChoices ) / sizeof( wxString );
	m_cbLayerPresets = new wxChoice( presetsSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbLayerPresetsNChoices, m_cbLayerPresetsChoices, 0 );
	m_cbLayerPresets->SetSelection( 1 );
	presetsSizer->Add( m_cbLayerPresets, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_btnDeletePreset = new wxBitmapButton( presetsSizer->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnDeletePreset->Enable( false );
	m_btnDeletePreset->SetToolTip( wxT("Delete this layer preset") );

	presetsSizer->Add( m_btnDeletePreset, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxTOP, 5 );


	m_sizerOuter->Add( presetsSizer, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	this->SetSizer( m_sizerOuter );
	this->Layout();

	// Connect Events
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( APPEARANCE_CONTROLS_BASE::OnNotebookPageChanged ), NULL, this );
	m_paneLayerDisplay->Connect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnLayerDisplayPaneChanged ), NULL, this );
	m_cbFlipBoard->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnFlipBoardChecked ), NULL, this );
	m_paneNetDisplay->Connect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnNetDisplayPaneChanged ), NULL, this );
	m_cbLayerPresets->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnLayerPresetChanged ), NULL, this );
	m_btnDeletePreset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnBtnDeleteLayerPreset ), NULL, this );
}

APPEARANCE_CONTROLS_BASE::~APPEARANCE_CONTROLS_BASE()
{
	// Disconnect Events
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( APPEARANCE_CONTROLS_BASE::OnNotebookPageChanged ), NULL, this );
	m_paneLayerDisplay->Disconnect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnLayerDisplayPaneChanged ), NULL, this );
	m_cbFlipBoard->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnFlipBoardChecked ), NULL, this );
	m_paneNetDisplay->Disconnect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnNetDisplayPaneChanged ), NULL, this );
	m_cbLayerPresets->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnLayerPresetChanged ), NULL, this );
	m_btnDeletePreset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnBtnDeleteLayerPreset ), NULL, this );

}
