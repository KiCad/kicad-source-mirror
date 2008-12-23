///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_graphic_item_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DialogGraphicItemProperties_base::DialogGraphicItemProperties_base( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );
	
	m_Start_Center_XText = new wxStaticText( this, wxID_ANY, wxT("Start Position X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Start_Center_XText->Wrap( -1 );
	bLeftSizer->Add( m_Start_Center_XText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Center_StartXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_Center_StartXCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_Start_Center_YText = new wxStaticText( this, wxID_ANY, wxT("Start Position Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Start_Center_YText->Wrap( -1 );
	bLeftSizer->Add( m_Start_Center_YText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Center_StartYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_Center_StartYCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_EndX_Radius_Text = new wxStaticText( this, wxID_ANY, wxT("End Position X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EndX_Radius_Text->Wrap( -1 );
	bLeftSizer->Add( m_EndX_Radius_Text, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_EndX_Radius_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_EndX_Radius_Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_EndY_Text = new wxStaticText( this, wxID_ANY, wxT("End Position Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EndY_Text->Wrap( -1 );
	bLeftSizer->Add( m_EndY_Text, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_EndY_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_EndY_Ctrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_Angle_Text = new wxStaticText( this, wxID_ANY, wxT("Arc Angle (0.1 degree)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Angle_Text->Wrap( -1 );
	bLeftSizer->Add( m_Angle_Text, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_Angle_Ctrl = new wxTextCtrl( this, wxID_ANGLE_CTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bLeftSizer->Add( m_Angle_Ctrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bMainSizer->Add( bLeftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxVERTICAL );
	
	m_ItemThicknessText = new wxStaticText( this, wxID_ANY, wxT("Item Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ItemThicknessText->Wrap( -1 );
	bMiddleSizer->Add( m_ItemThicknessText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessCtrl->SetToolTip( wxT("Thickness of this item") );
	
	bMiddleSizer->Add( m_ThicknessCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_DefaultThicknessText = new wxStaticText( this, wxID_ANY, wxT("Default Thickness"), wxDefaultPosition, wxDefaultSize, 0 );
	m_DefaultThicknessText->Wrap( -1 );
	bMiddleSizer->Add( m_DefaultThicknessText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_DefaultThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_DefaultThicknessCtrl->SetToolTip( wxT("Default value for thickness when creating a new graphic item") );
	
	bMiddleSizer->Add( m_DefaultThicknessCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMiddleSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_LayerText = new wxStaticText( this, wxID_ANY, wxT("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerText->Wrap( -1 );
	bMiddleSizer->Add( m_LayerText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxArrayString m_LayerSelectionChoices;
	m_LayerSelection = new wxChoice( this, wxID_LAYER_SELECTION, wxDefaultPosition, wxDefaultSize, m_LayerSelectionChoices, 0 );
	m_LayerSelection->SetSelection( 0 );
	bMiddleSizer->Add( m_LayerSelection, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	bMainSizer->Add( bMiddleSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bRightSizer->Add( m_buttonOK, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCANCEL, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	bMainSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	
	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DialogGraphicItemProperties_base::OnInitDialog ) );
	m_LayerSelection->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogGraphicItemProperties_base::OnLayerChoice ), NULL, this );
	m_buttonOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGraphicItemProperties_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGraphicItemProperties_base::OnCancelClick ), NULL, this );
}

DialogGraphicItemProperties_base::~DialogGraphicItemProperties_base()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DialogGraphicItemProperties_base::OnInitDialog ) );
	m_LayerSelection->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogGraphicItemProperties_base::OnLayerChoice ), NULL, this );
	m_buttonOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGraphicItemProperties_base::OnOkClick ), NULL, this );
	m_buttonCANCEL->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogGraphicItemProperties_base::OnCancelClick ), NULL, this );
}
