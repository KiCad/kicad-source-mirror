///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 29 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_layers_setup_base2.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_LAYERS_SETUP_BASE2::DIALOG_LAYERS_SETUP_BASE2( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bChoicesSizer;
	bChoicesSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbPresetsSizer;
	sbPresetsSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Preset Layer Groupings") ), wxVERTICAL );
	
	wxString m_PresetsChoiceChoices[] = { wxT("All Layers On"), wxT("Single Sided"), wxT("Single Sided, SMD on Back"), wxT("Two Layers, Parts on Front"), wxT("Two Layers, Parts on Both Faces"), wxT("Four Layers, Parts on Front"), wxT("Four Layers, Parts on Both Faces") };
	int m_PresetsChoiceNChoices = sizeof( m_PresetsChoiceChoices ) / sizeof( wxString );
	m_PresetsChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_PresetsChoiceNChoices, m_PresetsChoiceChoices, 0 );
	m_PresetsChoice->SetSelection( 0 );
	sbPresetsSizer->Add( m_PresetsChoice, 0, wxEXPAND, 5 );
	
	bChoicesSizer->Add( sbPresetsSizer, 2, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbCopperLayersSizer;
	sbCopperLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Copper Layers") ), wxVERTICAL );
	
	wxString m_CopperLayersChoiceChoices[] = { wxT("1"), wxT("2"), wxT("4"), wxT("6"), wxT("8"), wxT("10"), wxT("12"), wxT("14"), wxT("16") };
	int m_CopperLayersChoiceNChoices = sizeof( m_CopperLayersChoiceChoices ) / sizeof( wxString );
	m_CopperLayersChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_CopperLayersChoiceNChoices, m_CopperLayersChoiceChoices, 0 );
	m_CopperLayersChoice->SetSelection( 0 );
	sbCopperLayersSizer->Add( m_CopperLayersChoice, 0, wxEXPAND, 5 );
	
	bChoicesSizer->Add( sbCopperLayersSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	bMainSizer->Add( bChoicesSizer, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbLayersSizer;
	sbLayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Layers") ), wxVERTICAL );
	
	wxBoxSizer* bCaptionsSizer;
	bCaptionsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_LayerNameCaption = new wxStaticText( this, wxID_ANY, wxT("Name"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_CENTRE );
	m_LayerNameCaption->Wrap( -1 );
	m_LayerNameCaption->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bCaptionsSizer->Add( m_LayerNameCaption, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	m_LayerEnabledCaption = new wxStaticText( this, wxID_ANY, wxT("Enabled"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_CENTRE );
	m_LayerEnabledCaption->Wrap( -1 );
	m_LayerEnabledCaption->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bCaptionsSizer->Add( m_LayerEnabledCaption, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
	
	m_LayerTypeCaption = new wxStaticText( this, wxID_ANY, wxT("Type"), wxDefaultPosition, wxSize( -1,-1 ), wxALIGN_CENTRE );
	m_LayerTypeCaption->Wrap( -1 );
	m_LayerTypeCaption->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bCaptionsSizer->Add( m_LayerTypeCaption, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sbLayersSizer->Add( bCaptionsSizer, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );
	
	m_LayersListPanel = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxALWAYS_SHOW_SB|wxRAISED_BORDER|wxTAB_TRAVERSAL|wxVSCROLL );
	m_LayersListPanel->SetScrollRate( 0, 5 );
	m_LayerListFlexGridSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	m_LayerListFlexGridSizer->AddGrowableCol( 0 );
	m_LayerListFlexGridSizer->AddGrowableCol( 1 );
	m_LayerListFlexGridSizer->AddGrowableCol( 2 );
	m_LayerListFlexGridSizer->SetFlexibleDirection( wxHORIZONTAL );
	m_LayerListFlexGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_junkStaticText = new wxStaticText( m_LayersListPanel, wxID_ANY, wxT("MyLabel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_junkStaticText->Wrap( -1 );
	m_LayerListFlexGridSizer->Add( m_junkStaticText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_junkCheckBox = new wxCheckBox( m_LayersListPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayerListFlexGridSizer->Add( m_junkCheckBox, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	wxArrayString m_junkChoiceChoices;
	m_junkChoice = new wxChoice( m_LayersListPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_junkChoiceChoices, 0 );
	m_junkChoice->SetSelection( 0 );
	m_LayerListFlexGridSizer->Add( m_junkChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_LayersListPanel->SetSizer( m_LayerListFlexGridSizer );
	m_LayersListPanel->Layout();
	m_LayerListFlexGridSizer->Fit( m_LayersListPanel );
	sbLayersSizer->Add( m_LayersListPanel, 1, wxALL|wxEXPAND, 5 );
	
	bMainSizer->Add( sbLayersSizer, 1, wxALIGN_CENTER_HORIZONTAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );
	
	m_sdbSizer2 = new wxStdDialogButtonSizer();
	m_sdbSizer2OK = new wxButton( this, wxID_OK );
	m_sdbSizer2->AddButton( m_sdbSizer2OK );
	m_sdbSizer2Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer2->AddButton( m_sdbSizer2Cancel );
	m_sdbSizer2->Realize();
	bMainSizer->Add( m_sdbSizer2, 0, wxALL|wxEXPAND, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
}

DIALOG_LAYERS_SETUP_BASE2::~DIALOG_LAYERS_SETUP_BASE2()
{
}
