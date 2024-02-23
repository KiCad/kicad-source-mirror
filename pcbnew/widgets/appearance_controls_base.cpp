///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/wx_grid.h"

#include "appearance_controls_base.h"

///////////////////////////////////////////////////////////////////////////

APPEARANCE_CONTROLS_BASE::APPEARANCE_CONTROLS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : WX_PANEL( parent, id, pos, size, style, name )
{
	this->SetMinSize( wxSize( 200,360 ) );

	m_sizerOuter = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_panelLayers = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelLayersSizer = new wxBoxSizer( wxVERTICAL );

	m_windowLayers = new wxScrolledCanvas( m_panelLayers, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_panelLayersSizer->Add( m_windowLayers, 1, wxEXPAND, 5 );


	m_panelLayers->SetSizer( m_panelLayersSizer );
	m_panelLayers->Layout();
	m_panelLayersSizer->Fit( m_panelLayers );
	m_notebook->AddPage( m_panelLayers, _("Layers"), true );
	m_panelObjects = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panelObjects->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_objectsPanelSizer = new wxBoxSizer( wxVERTICAL );

	m_windowObjects = new wxScrolledCanvas( m_panelObjects, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_objectsPanelSizer->Add( m_windowObjects, 1, wxEXPAND, 5 );


	m_panelObjects->SetSizer( m_objectsPanelSizer );
	m_panelObjects->Layout();
	m_objectsPanelSizer->Fit( m_panelObjects );
	m_notebook->AddPage( m_panelObjects, _("Objects"), false );
	m_panelNetsAndClasses = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_netsTabOuterSizer = new wxBoxSizer( wxVERTICAL );

	m_netsTabSplitter = new wxSplitterWindow( m_panelNetsAndClasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_netsTabSplitter->SetSashGravity( 0.8 );
	m_netsTabSplitter->Connect( wxEVT_IDLE, wxIdleEventHandler( APPEARANCE_CONTROLS_BASE::m_netsTabSplitterOnIdle ), NULL, this );
	m_netsTabSplitter->SetMinimumPaneSize( 80 );

	m_panelNets = new wxPanel( m_netsTabSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer192;
	bSizer192 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextNets = new wxStaticText( m_panelNets, wxID_ANY, _("Nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNets->Wrap( -1 );
	bSizer17->Add( m_staticTextNets, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_txtNetFilter = new wxTextCtrl( m_panelNets, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_txtNetFilter->Hide();

	bSizer17->Add( m_txtNetFilter, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_btnNetInspector = new BITMAP_BUTTON( m_panelNets, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnNetInspector->SetToolTip( _("Show the Net Inspector") );

	bSizer17->Add( m_btnNetInspector, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizer192->Add( bSizer17, 0, wxEXPAND, 5 );

	m_netsGrid = new WX_GRID( m_panelNets, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_netsGrid->CreateGrid( 5, 3 );
	m_netsGrid->EnableEditing( false );
	m_netsGrid->EnableGridLines( false );
	m_netsGrid->EnableDragGridSize( false );
	m_netsGrid->SetMargins( 0, 0 );

	// Columns
	m_netsGrid->SetColSize( 0, 40 );
	m_netsGrid->SetColSize( 1, 40 );
	m_netsGrid->SetColSize( 2, 400 );
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
	m_netsGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	bSizer192->Add( m_netsGrid, 0, wxALL|wxEXPAND, 5 );


	m_panelNets->SetSizer( bSizer192 );
	m_panelNets->Layout();
	bSizer192->Fit( m_panelNets );
	m_panelNetclasses = new wxPanel( m_netsTabSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerNetClasses;
	bSizerNetClasses = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer20;
	bSizer20 = new wxBoxSizer( wxHORIZONTAL );

	m_staticTextNetClasses = new wxStaticText( m_panelNetclasses, wxID_ANY, _("Net Classes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextNetClasses->Wrap( -1 );
	bSizer20->Add( m_staticTextNetClasses, 1, wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_btnConfigureNetClasses = new BITMAP_BUTTON( m_panelNetclasses, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_btnConfigureNetClasses->SetToolTip( _("Configure net classes") );

	bSizer20->Add( m_btnConfigureNetClasses, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


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
	m_netsTabOuterSizer->Add( m_netsTabSplitter, 1, wxEXPAND, 5 );


	m_panelNetsAndClasses->SetSizer( m_netsTabOuterSizer );
	m_panelNetsAndClasses->Layout();
	m_netsTabOuterSizer->Fit( m_panelNetsAndClasses );
	m_notebook->AddPage( m_panelNetsAndClasses, _("Nets"), false );

	m_sizerOuter->Add( m_notebook, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bBottomMargin;
	bBottomMargin = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bPresets;
	bPresets = new wxBoxSizer( wxVERTICAL );

	m_presetsLabel = new wxStaticText( this, wxID_ANY, _("Presets (Ctrl+Tab):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_presetsLabel->Wrap( -1 );
	bPresets->Add( m_presetsLabel, 1, wxTOP|wxRIGHT|wxLEFT, 2 );

	wxString m_cbLayerPresetsChoices[] = { _("All Layers"), _("(unsaved)") };
	int m_cbLayerPresetsNChoices = sizeof( m_cbLayerPresetsChoices ) / sizeof( wxString );
	m_cbLayerPresets = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbLayerPresetsNChoices, m_cbLayerPresetsChoices, 0 );
	m_cbLayerPresets->SetSelection( 1 );
	bPresets->Add( m_cbLayerPresets, 0, wxALL|wxEXPAND, 2 );


	bBottomMargin->Add( bPresets, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bBottomMargin->Add( 0, 2, 0, wxEXPAND, 5 );

	wxBoxSizer* bViewports;
	bViewports = new wxBoxSizer( wxVERTICAL );

	m_viewportsLabel = new wxStaticText( this, wxID_ANY, _("Viewports (Alt+Tab):"), wxDefaultPosition, wxDefaultSize, 0 );
	m_viewportsLabel->Wrap( -1 );
	bViewports->Add( m_viewportsLabel, 1, wxRIGHT|wxLEFT, 2 );

	wxString m_cbViewportsChoices[] = { _("(unsaved)") };
	int m_cbViewportsNChoices = sizeof( m_cbViewportsChoices ) / sizeof( wxString );
	m_cbViewports = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_cbViewportsNChoices, m_cbViewportsChoices, 0 );
	m_cbViewports->SetSelection( 1 );
	bViewports->Add( m_cbViewports, 0, wxALL|wxEXPAND, 2 );


	bBottomMargin->Add( bViewports, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	m_sizerOuter->Add( bBottomMargin, 0, wxEXPAND|wxTOP|wxBOTTOM, 4 );


	this->SetSizer( m_sizerOuter );
	this->Layout();
	m_sizerOuter->Fit( this );

	// Connect Events
	this->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( APPEARANCE_CONTROLS_BASE::OnSize ) );
	m_notebook->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( APPEARANCE_CONTROLS_BASE::OnNotebookPageChanged ), NULL, this );
	m_notebook->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelLayers->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelObjects->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetsAndClasses->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNets->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_netsGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridClick ), NULL, this );
	m_netsGrid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridDoubleClick ), NULL, this );
	m_netsGrid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridRightClick ), NULL, this );
	m_netsGrid->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetclasses->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_cbLayerPresets->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::onLayerPresetChanged ), NULL, this );
	m_cbViewports->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::onViewportChanged ), NULL, this );
}

APPEARANCE_CONTROLS_BASE::~APPEARANCE_CONTROLS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( APPEARANCE_CONTROLS_BASE::OnSize ) );
	m_notebook->Disconnect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( APPEARANCE_CONTROLS_BASE::OnNotebookPageChanged ), NULL, this );
	m_notebook->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelLayers->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelObjects->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetsAndClasses->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNets->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridClick ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridDoubleClick ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( APPEARANCE_CONTROLS_BASE::OnNetGridRightClick ), NULL, this );
	m_netsGrid->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_panelNetclasses->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( APPEARANCE_CONTROLS_BASE::OnSetFocus ), NULL, this );
	m_cbLayerPresets->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::onLayerPresetChanged ), NULL, this );
	m_cbViewports->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( APPEARANCE_CONTROLS_BASE::onViewportChanged ), NULL, this );

}
