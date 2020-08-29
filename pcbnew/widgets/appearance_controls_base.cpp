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
	this->SetFont( wxFont( 10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );
	this->SetMinSize( wxSize( 160,360 ) );

	m_sizerOuter = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelLayers = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLayersSizer = new wxBoxSizer( wxVERTICAL );

	m_windowLayers = new wxScrolledCanvas( m_panelLayers, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_panelLayersSizer->Add( m_windowLayers, 1, wxEXPAND, 5 );

	m_paneLayerDisplay = new wxCollapsiblePane( m_panelLayers, wxID_ANY, wxT("Layer Display Options"), wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE|wxCP_NO_TLW_RESIZE );
	m_paneLayerDisplay->Collapse( true );

	wxBoxSizer* bSizer121;
	bSizer121 = new wxBoxSizer( wxVERTICAL );

	m_staticText13 = new wxStaticText( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Non-active layers:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText13->Wrap( -1 );
	bSizer121->Add( m_staticText13, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	wxBoxSizer* bSizer19;
	bSizer19 = new wxBoxSizer( wxHORIZONTAL );

	m_rbHighContrastNormal = new wxRadioButton( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Normal"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbHighContrastNormal->SetValue( true );
	m_rbHighContrastNormal->SetToolTip( wxT("Non-active layers will be shown in full color") );

	bSizer19->Add( m_rbHighContrastNormal, 0, wxRIGHT|wxLEFT, 5 );

	m_rbHighContrastDim = new wxRadioButton( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Dim"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbHighContrastDim->SetToolTip( wxT("Non-active layers will be dimmed") );

	bSizer19->Add( m_rbHighContrastDim, 0, wxRIGHT|wxLEFT, 5 );

	m_rbHighContrastOff = new wxRadioButton( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Hide"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbHighContrastOff->SetToolTip( wxT("Non-active layers will be hidden") );

	bSizer19->Add( m_rbHighContrastOff, 0, wxRIGHT|wxLEFT, 5 );


	bSizer121->Add( bSizer19, 0, wxEXPAND, 5 );

	m_staticline5 = new wxStaticLine( m_paneLayerDisplay->GetPane(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer121->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );

	m_cbFlipBoard = new wxCheckBox( m_paneLayerDisplay->GetPane(), wxID_ANY, wxT("Flip board view"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer121->Add( m_cbFlipBoard, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_paneLayerDisplay->GetPane()->SetSizer( bSizer121 );
	m_paneLayerDisplay->GetPane()->Layout();
	bSizer121->Fit( m_paneLayerDisplay->GetPane() );
	m_panelLayersSizer->Add( m_paneLayerDisplay, 0, wxBOTTOM|wxTOP|wxEXPAND, 5 );


	m_panelLayers->SetSizer( m_panelLayersSizer );
	m_panelLayers->Layout();
	m_panelLayersSizer->Fit( m_panelLayers );
	m_notebook->AddPage( m_panelLayers, wxT("Layers"), true );
	m_panelObjects = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelObjects->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_objectsPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_windowObjects = new wxScrolledCanvas( m_panelObjects, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_objectsPanelSizer->Add( m_windowObjects, 1, wxEXPAND, 5 );


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

	m_staticTextNets = new wxStaticText( m_panelNets, wxID_ANY, wxT("Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNets->Wrap( -1 );
	m_staticTextNets->SetFont( wxFont( 9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer17->Add( m_staticTextNets, 1, wxALL, 5 );

	m_txtNetFilter = new wxTextCtrl( m_panelNets, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtNetFilter->Hide();

	bSizer17->Add( m_txtNetFilter, 1, wxLEFT|wxTOP, 5 );

	m_btnNetInspector = new wxBitmapButton( m_panelNets, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_btnNetInspector->SetToolTip( wxT("Show the Net Inspector") );

	bSizer17->Add( m_btnNetInspector, 0, wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizer192->Add( bSizer17, 0, wxEXPAND, 5 );

	m_netsGrid = new wxGrid( m_panelNets, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_netsGrid->CreateGrid( 5, 3 );
	m_netsGrid->EnableEditing( false );
	m_netsGrid->EnableGridLines( false );
	m_netsGrid->EnableDragGridSize( false );
	m_netsGrid->SetMargins( 0, 0 );

	// Columns
	m_netsGrid->AutoSizeColumns();
	m_netsGrid->EnableDragColMove( false );
	m_netsGrid->EnableDragColSize( false );
	m_netsGrid->SetColLabelSize( 0 );
	m_netsGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_netsGrid->EnableDragRowSize( false );
	m_netsGrid->SetRowLabelSize( 0 );
	m_netsGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_netsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer192->Add( m_netsGrid, 1, wxALL|wxEXPAND, 5 );


	m_panelNets->SetSizer( bSizer192 );
	m_panelNets->Layout();
	bSizer192->Fit( m_panelNets );
	m_panelNetclasses = new wxPanel( m_netsTabSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerNetClasses;
	bSizerNetClasses = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText14 = new wxStaticText( m_panelNetclasses, wxID_ANY, wxT("Net Classes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText14->Wrap( -1 );
	m_staticText14->SetFont( wxFont( 9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer20->Add( m_staticText14, 1, wxALL, 5 );

	m_btnConfigureNetClasses = new wxBitmapButton( m_panelNetclasses, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_btnConfigureNetClasses->SetToolTip( wxT("Configure net classes") );

	bSizer20->Add( m_btnConfigureNetClasses, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizerNetClasses->Add( bSizer20, 0, wxEXPAND, 5 );

	m_netclassScrolledWindow = new wxScrolledWindow( m_panelNetclasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_netclassScrolledWindow->SetScrollRate( 5, 5 );
	m_netclassOuterSizer = new wxBoxSizer( wxVERTICAL );


	m_netclassScrolledWindow->SetSizer( m_netclassOuterSizer );
	m_netclassScrolledWindow->Layout();
	m_netclassOuterSizer->Fit( m_netclassScrolledWindow );
	bSizerNetClasses->Add( m_netclassScrolledWindow, 1, wxEXPAND | wxALL, 5 );


	m_panelNetclasses->SetSizer( bSizerNetClasses );
	m_panelNetclasses->Layout();
	bSizerNetClasses->Fit( m_panelNetclasses );
	m_netsTabSplitter->SplitHorizontally( m_panelNets, m_panelNetclasses, 300 );
	bSizer16->Add( m_netsTabSplitter, 1, wxEXPAND, 5 );

	m_paneNetDisplay = new wxCollapsiblePane( m_panelNetsAndClasses, wxID_ANY, wxT("Net Display Options"), wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE|wxCP_NO_TLW_RESIZE );
	m_paneNetDisplay->Collapse( true );

	wxBoxSizer* bSizerNetDisplay;
	bSizerNetDisplay = new wxBoxSizer( wxVERTICAL );

	m_staticTextNetDsplayTitle = new wxStaticText( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("Net colors:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetDsplayTitle->Wrap( -1 );
	m_staticTextNetDsplayTitle->SetToolTip( wxT("Choose when to show net and netclass colors") );

	bSizerNetDisplay->Add( m_staticTextNetDsplayTitle, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer191;
	bSizer191 = new wxBoxSizer( wxHORIZONTAL );

	m_rbNetColorAll = new wxRadioButton( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_rbNetColorAll->SetToolTip( wxT("Net and netclass colors are shown on all copper items") );

	bSizer191->Add( m_rbNetColorAll, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_rbNetColorRatsnest = new wxRadioButton( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("Ratsnest"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbNetColorRatsnest->SetValue( true );
	m_rbNetColorRatsnest->SetToolTip( wxT("Net and netclass colors are shown on the ratsnest only") );

	bSizer191->Add( m_rbNetColorRatsnest, 0, wxBOTTOM|wxLEFT, 5 );

	m_rbNetColorOff = new wxRadioButton( m_paneNetDisplay->GetPane(), wxID_ANY, wxT("None"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbNetColorOff->SetToolTip( wxT("Net and netclass colors are not shown") );

	bSizer191->Add( m_rbNetColorOff, 0, wxBOTTOM|wxLEFT, 5 );


	bSizerNetDisplay->Add( bSizer191, 0, wxEXPAND, 5 );


	m_paneNetDisplay->GetPane()->SetSizer( bSizerNetDisplay );
	m_paneNetDisplay->GetPane()->Layout();
	bSizerNetDisplay->Fit( m_paneNetDisplay->GetPane() );
	bSizer16->Add( m_paneNetDisplay, 0, wxEXPAND|wxTOP, 5 );


	m_panelNetsAndClasses->SetSizer( bSizer16 );
	m_panelNetsAndClasses->Layout();
	bSizer16->Fit( m_panelNetsAndClasses );
	m_notebook->AddPage( m_panelNetsAndClasses, wxT("Nets"), false );

	m_sizerOuter->Add( m_notebook, 1, wxEXPAND, 5 );

	wxBoxSizer* bBottomMargin;
	bBottomMargin = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bPresets;
	bPresets = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bPresetsLabels;
	bPresetsLabels = new wxBoxSizer( wxHORIZONTAL );

	presetsLabel = new wxStaticText( this, wxID_ANY, wxT("Presets:"), wxDefaultPosition, wxDefaultSize, 0 );
	presetsLabel->Wrap( -1 );
	bPresetsLabels->Add( presetsLabel, 1, wxTOP|wxRIGHT|wxLEFT, 2 );

	presetsHotkey = new wxStaticText( this, wxID_ANY, wxT("(Crtl+Tab)"), wxDefaultPosition, wxDefaultSize, 0 );
	presetsHotkey->Wrap( -1 );
	bPresetsLabels->Add( presetsHotkey, 0, wxTOP|wxRIGHT|wxLEFT, 2 );


	bPresets->Add( bPresetsLabels, 1, wxEXPAND, 5 );

	wxString m_cbLayerPresetsChoices[] = { wxT("All Layers"), wxT("(unsaved)") };
	int m_cbLayerPresetsNChoices = sizeof( m_cbLayerPresetsChoices ) / sizeof( wxString );
	m_cbLayerPresets = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbLayerPresetsNChoices, m_cbLayerPresetsChoices, 0 );
	m_cbLayerPresets->SetSelection( 1 );
	m_cbLayerPresets->SetToolTip( wxT("Layer presets") );

	bPresets->Add( m_cbLayerPresets, 0, wxALL|wxEXPAND, 2 );


	bBottomMargin->Add( bPresets, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	m_sizerOuter->Add( bBottomMargin, 0, wxBOTTOM|wxEXPAND, 2 );


	this->SetSizer( m_sizerOuter );
	this->Layout();
	m_sizerOuter->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( APPEARANCE_CONTROLS_BASE::OnSize ) );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( APPEARANCE_CONTROLS_BASE::OnNotebookPageChanged ), NULL, this );
	m_notebook->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelLayers->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_paneLayerDisplay->Connect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnLayerDisplayPaneChanged ), NULL, this );
	m_paneLayerDisplay->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_cbFlipBoard->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnFlipBoardChecked ), NULL, this );
	m_panelObjects->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetsAndClasses->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNets->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_netsGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridClick ), NULL, this );
	m_netsGrid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridDoubleClick ), NULL, this );
	m_netsGrid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridRightClick ), NULL, this );
	m_netsGrid->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetclasses->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_paneNetDisplay->Connect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnNetDisplayPaneChanged ), NULL, this );
	m_paneNetDisplay->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_cbLayerPresets->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::onLayerPresetChanged ), NULL, this );
}

APPEARANCE_CONTROLS_BASE::~APPEARANCE_CONTROLS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( APPEARANCE_CONTROLS_BASE::OnSize ) );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( APPEARANCE_CONTROLS_BASE::OnNotebookPageChanged ), NULL, this );
	m_notebook->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelLayers->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_paneLayerDisplay->Disconnect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnLayerDisplayPaneChanged ), NULL, this );
	m_paneLayerDisplay->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_cbFlipBoard->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::OnFlipBoardChecked ), NULL, this );
	m_panelObjects->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetsAndClasses->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNets->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridClick ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridDoubleClick ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridRightClick ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetclasses->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_paneNetDisplay->Disconnect( wxEVT_COLLAPSIBLEPANE_CHANGED, wxCollapsiblePaneEventHandler( APPEARANCE_CONTROLS_BASE::OnNetDisplayPaneChanged ), NULL, this );
	m_paneNetDisplay->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_cbLayerPresets->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::onLayerPresetChanged ), NULL, this );

}
