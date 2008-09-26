///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_non_copper_zones_properties.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DialogNonCopperZonesProperties, wxDialog )
	EVT_INIT_DIALOG( DialogNonCopperZonesProperties::_wxFB_InitDialog )
	EVT_BUTTON( wxID_OK, DialogNonCopperZonesProperties::_wxFB_OnOkClick )
	EVT_BUTTON( wxID_CANCEL, DialogNonCopperZonesProperties::_wxFB_OnCancelClick )
END_EVENT_TABLE()

DialogNonCopperZonesProperties::DialogNonCopperZonesProperties( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_UpperSizer;
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched Outline"), _("Full Hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, wxID_ANY, _("Outlines Appearence"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 1 );
	m_UpperSizer->Add( m_OutlineAppearanceCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any"), _("H, V and 45 deg") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone Edges Orient"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 0 );
	m_UpperSizer->Add( m_OrientEdgesOpt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
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

DialogNonCopperZonesProperties::~DialogNonCopperZonesProperties()
{
}
