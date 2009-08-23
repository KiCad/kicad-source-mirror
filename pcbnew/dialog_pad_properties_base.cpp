///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_pad_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DialogPadPropertiesBase::DialogPadPropertiesBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_LeftBoxSizer;
	m_LeftBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_PadNumText = new wxStaticText( this, wxID_ANY, _("Pad Num :"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNumText->Wrap( -1 );
	m_LeftBoxSizer->Add( m_PadNumText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadNumCtrl = new wxTextCtrl( this, wxID_PADNUMCTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LeftBoxSizer->Add( m_PadNumCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_PadNameText = new wxStaticText( this, wxID_ANY, _("Pad Net Name :"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadNameText->Wrap( -1 );
	m_LeftBoxSizer->Add( m_PadNameText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadNetNameCtrl = new wxTextCtrl( this, wxID_PADNETNAMECTRL, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_LeftBoxSizer->Add( m_PadNetNameCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_PadPositionBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_LeftBoxSizer->Add( m_PadPositionBoxSizer, 0, wxEXPAND, 5 );
	
	m_MainSizer->Add( m_LeftBoxSizer, 0, wxBOTTOM|wxLEFT, 5 );
	
	m_DrillShapeBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PadShapeChoices[] = { _("Circle"), _("Oval"), _("Rect"), _("Trapezoidal") };
	int m_PadShapeNChoices = sizeof( m_PadShapeChoices ) / sizeof( wxString );
	m_PadShape = new wxRadioBox( this, ID_LISTBOX_SHAPE_PAD, _("Pad Shape:"), wxDefaultPosition, wxDefaultSize, m_PadShapeNChoices, m_PadShapeChoices, 1, wxRA_SPECIFY_COLS );
	m_PadShape->SetSelection( 0 );
	m_DrillShapeBoxSizer->Add( m_PadShape, 0, wxALL|wxEXPAND, 5 );
	
	
	m_DrillShapeBoxSizer->Add( 0, 8, 1, wxEXPAND, 5 );
	
	wxString m_DrillShapeCtrlChoices[] = { _("Circle"), _("Oval") };
	int m_DrillShapeCtrlNChoices = sizeof( m_DrillShapeCtrlChoices ) / sizeof( wxString );
	m_DrillShapeCtrl = new wxRadioBox( this, ID_RADIOBOX_DRILL_SHAPE, _("Drill Shape:"), wxDefaultPosition, wxDefaultSize, m_DrillShapeCtrlNChoices, m_DrillShapeCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_DrillShapeCtrl->SetSelection( 1 );
	m_DrillShapeBoxSizer->Add( m_DrillShapeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_MainSizer->Add( m_DrillShapeBoxSizer, 1, wxBOTTOM, 5 );
	
	wxBoxSizer* m_MiddleRightBoxSizer;
	m_MiddleRightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PadOrientChoices[] = { _("0"), _("90"), _("-90"), _("180"), _("User") };
	int m_PadOrientNChoices = sizeof( m_PadOrientChoices ) / sizeof( wxString );
	m_PadOrient = new wxRadioBox( this, ID_LISTBOX_ORIENT_PAD, _("Pad Orient:"), wxDefaultPosition, wxDefaultSize, m_PadOrientNChoices, m_PadOrientChoices, 1, wxRA_SPECIFY_COLS );
	m_PadOrient->SetSelection( 0 );
	m_MiddleRightBoxSizer->Add( m_PadOrient, 0, wxALL|wxEXPAND, 5 );
	
	m_PadOrientText = new wxStaticText( this, wxID_ANY, _("Pad Orient (0.1 deg)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PadOrientText->Wrap( -1 );
	m_MiddleRightBoxSizer->Add( m_PadOrientText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadOrientCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_MiddleRightBoxSizer->Add( m_PadOrientCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	m_MiddleRightBoxSizer->Add( 0, 8, 1, wxEXPAND, 5 );
	
	wxString m_PadTypeChoices[] = { _("Standard"), _("SMD"), _("Conn") };
	int m_PadTypeNChoices = sizeof( m_PadTypeChoices ) / sizeof( wxString );
	m_PadType = new wxRadioBox( this, ID_LISTBOX_TYPE_PAD, _("Pad Type:"), wxDefaultPosition, wxDefaultSize, m_PadTypeNChoices, m_PadTypeChoices, 1, wxRA_SPECIFY_COLS );
	m_PadType->SetSelection( 0 );
	m_MiddleRightBoxSizer->Add( m_PadType, 0, wxALL|wxEXPAND, 5 );
	
	m_MainSizer->Add( m_MiddleRightBoxSizer, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* m_RightBoxSizer;
	m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOk = new wxButton( this, wxID_OK, _("Ok"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOk->SetDefault(); 
	m_RightBoxSizer->Add( m_buttonOk, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RightBoxSizer->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxEXPAND, 5 );
	
	
	m_RightBoxSizer->Add( 0, 8, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* m_LayersSizer;
	m_LayersSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Layers:") ), wxVERTICAL );
	
	m_PadLayerCu = new wxCheckBox( this, wxID_ANY, _("Copper layer"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerCmp = new wxCheckBox( this, wxID_ANY, _("Component layer"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerCmp, 0, wxALL, 5 );
	
	
	m_LayersSizer->Add( 0, 8, 1, wxEXPAND, 5 );
	
	m_PadLayerAdhCmp = new wxCheckBox( this, wxID_ANY, _("Adhesive Cmp"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerAdhCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerAdhCu = new wxCheckBox( this, wxID_ANY, _("Adhesive Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerAdhCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerPateCmp = new wxCheckBox( this, wxID_ANY, _("Solder paste Cmp"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerPateCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerPateCu = new wxCheckBox( this, wxID_ANY, _("Solder paste Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerPateCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerSilkCmp = new wxCheckBox( this, wxID_ANY, _("Silkscreen Cmp"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerSilkCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerSilkCu = new wxCheckBox( this, wxID_ANY, _("Silkscreen Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerSilkCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerMaskCmp = new wxCheckBox( this, wxID_ANY, _("Solder mask Cmp"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerMaskCmp, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerMaskCu = new wxCheckBox( this, wxID_ANY, _("Solder mask Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerMaskCu, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerECO1 = new wxCheckBox( this, wxID_ANY, _("E.C.O.1 layer"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerECO1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerECO2 = new wxCheckBox( this, wxID_ANY, _("E.C.O.2 layer"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerECO2, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PadLayerDraft = new wxCheckBox( this, wxID_ANY, _("Draft layer"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_LayersSizer->Add( m_PadLayerDraft, 0, wxALL, 5 );
	
	m_RightBoxSizer->Add( m_LayersSizer, 0, 0, 5 );
	
	m_MainSizer->Add( m_RightBoxSizer, 0, wxBOTTOM|wxRIGHT, 5 );
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_PadShape->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::OnPadShapeSelection ), NULL, this );
	m_DrillShapeCtrl->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::OnDrillShapeSelected ), NULL, this );
	m_PadOrient->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::PadOrientEvent ), NULL, this );
	m_PadType->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::PadTypeSelected ), NULL, this );
	m_buttonOk->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogPadPropertiesBase::PadPropertiesAccept ), NULL, this );
	m_buttonCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogPadPropertiesBase::OnCancelButtonClick ), NULL, this );
}

DialogPadPropertiesBase::~DialogPadPropertiesBase()
{
	// Disconnect Events
	m_PadShape->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::OnPadShapeSelection ), NULL, this );
	m_DrillShapeCtrl->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::OnDrillShapeSelected ), NULL, this );
	m_PadOrient->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::PadOrientEvent ), NULL, this );
	m_PadType->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DialogPadPropertiesBase::PadTypeSelected ), NULL, this );
	m_buttonOk->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogPadPropertiesBase::PadPropertiesAccept ), NULL, this );
	m_buttonCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogPadPropertiesBase::OnCancelButtonClick ), NULL, this );
}
