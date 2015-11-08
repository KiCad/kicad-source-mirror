///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_keepout_area_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_KEEPOUT_AREA_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_CANCEL, DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::_wxFB_OnOkClick )
END_EVENT_TABLE()

DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_UpperSizer;
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* m_layersListSizer;
	m_layersListSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	m_layersListSizer->Add( m_staticTextLayerSelection, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LayerSelectionCtrl = new wxListView( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ALIGN_LEFT|wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	m_layersListSizer->Add( m_LayerSelectionCtrl, 1, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	m_UpperSizer->Add( m_layersListSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextprops = new wxStaticText( this, wxID_ANY, _("Properties:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextprops->Wrap( -1 );
	bSizerRight->Add( m_staticTextprops, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any"), _("180, 90, and 45 degrees") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone Edge Orientation:"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 1 );
	bSizerRight->Add( m_OrientEdgesOpt, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched Outline"), _("Full Hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, wxID_ANY, _("Outline Appearence:"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 1 );
	bSizerRight->Add( m_OutlineAppearanceCtrl, 0, wxALL|wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizerCutoutOpts;
	sbSizerCutoutOpts = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Keepout Options:") ), wxVERTICAL );
	
	m_cbTracksCtrl = new wxCheckBox( this, wxID_ANY, _("No tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerCutoutOpts->Add( m_cbTracksCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_cbViasCtrl = new wxCheckBox( this, wxID_ANY, _("No vias"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerCutoutOpts->Add( m_cbViasCtrl, 0, wxTOP|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_cbCopperPourCtrl = new wxCheckBox( this, wxID_ANY, _("No copper pour"), wxDefaultPosition, wxDefaultSize, 0 );
	sbSizerCutoutOpts->Add( m_cbCopperPourCtrl, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizerRight->Add( sbSizerCutoutOpts, 0, wxEXPAND|wxALL, 5 );
	
	
	m_UpperSizer->Add( bSizerRight, 0, wxEXPAND, 5 );
	
	
	m_MainSizer->Add( m_UpperSizer, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	m_MainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND|wxALL, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_KEEPOUT_AREA_PROPERTIES_BASE::~DIALOG_KEEPOUT_AREA_PROPERTIES_BASE()
{
}
