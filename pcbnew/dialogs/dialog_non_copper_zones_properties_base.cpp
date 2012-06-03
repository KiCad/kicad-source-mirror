///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 11 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_non_copper_zones_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DialogNonCopperZonesPropertiesBase, DIALOG_SHIM )
	EVT_BUTTON( wxID_OK, DialogNonCopperZonesPropertiesBase::_wxFB_OnOkClick )
	EVT_BUTTON( wxID_CANCEL, DialogNonCopperZonesPropertiesBase::_wxFB_OnCancelClick )
END_EVENT_TABLE()

DialogNonCopperZonesPropertiesBase::DialogNonCopperZonesPropertiesBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_UpperSizer;
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLeftSizer_;
	sbLeftSizer_ = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Zone Fill Options:") ), wxVERTICAL );
	
	wxString m_FillModeCtrlChoices[] = { _("Use polygons"), _("Use segments") };
	int m_FillModeCtrlNChoices = sizeof( m_FillModeCtrlChoices ) / sizeof( wxString );
	m_FillModeCtrl = new wxRadioBox( this, wxID_ANY, _("Filling Mode:"), wxDefaultPosition, wxDefaultSize, m_FillModeCtrlNChoices, m_FillModeCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_FillModeCtrl->SetSelection( 0 );
	sbLeftSizer_->Add( m_FillModeCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_MinThicknessValueTitle = new wxStaticText( this, wxID_ANY, _("Zone min thickness value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinThicknessValueTitle->Wrap( -1 );
	sbLeftSizer_->Add( m_MinThicknessValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneMinThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbLeftSizer_->Add( m_ZoneMinThicknessCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	m_UpperSizer->Add( sbLeftSizer_, 0, 0, 5 );
	
	wxStaticBoxSizer* m_OutilinesBoxOpt;
	m_OutilinesBoxOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Outlines Options:") ), wxVERTICAL );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any"), _("H, V and 45 deg") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone Edges Orient"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 0 );
	m_OutilinesBoxOpt->Add( m_OrientEdgesOpt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched Outline"), _("Full Hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, wxID_ANY, _("Outlines Appearence"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 1 );
	m_OutilinesBoxOpt->Add( m_OutlineAppearanceCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_UpperSizer->Add( m_OutilinesBoxOpt, 0, 0, 5 );
	
	wxBoxSizer* m_ButtonsSizer;
	m_ButtonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOk = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOk->SetDefault(); 
	m_ButtonsSizer->Add( m_buttonOk, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_buttonCancel = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ButtonsSizer->Add( m_buttonCancel, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	m_UpperSizer->Add( m_ButtonsSizer, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	
	m_MainSizer->Add( m_UpperSizer, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layer selection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	m_MainSizer->Add( m_staticTextLayerSelection, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LayerSelectionCtrl = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	m_MainSizer->Add( m_LayerSelectionCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DialogNonCopperZonesPropertiesBase::~DialogNonCopperZonesPropertiesBase()
{
}
