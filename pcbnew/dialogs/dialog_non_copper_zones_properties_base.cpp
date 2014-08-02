///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_non_copper_zones_properties_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE, DIALOG_SHIM )
	EVT_BUTTON( wxID_CANCEL, DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::_wxFB_OnCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::_wxFB_OnOkClick )
END_EVENT_TABLE()

DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* m_MainSizer;
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* m_UpperSizer;
	m_UpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizerLeft;
	bSizerLeft = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextLayerSelection = new wxStaticText( this, wxID_ANY, _("Layer selection:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextLayerSelection->Wrap( -1 );
	bSizerLeft->Add( m_staticTextLayerSelection, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_LayerSelectionCtrl = new wxListView( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_ALIGN_LEFT|wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	bSizerLeft->Add( m_LayerSelectionCtrl, 1, wxALL|wxEXPAND, 5 );
	
	
	m_UpperSizer->Add( bSizerLeft, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerRight;
	bSizerRight = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* m_OutilinesBoxOpt;
	m_OutilinesBoxOpt = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Outlines Options:") ), wxVERTICAL );
	
	wxString m_OrientEdgesOptChoices[] = { _("Any"), _("H, V and 45 deg") };
	int m_OrientEdgesOptNChoices = sizeof( m_OrientEdgesOptChoices ) / sizeof( wxString );
	m_OrientEdgesOpt = new wxRadioBox( this, wxID_ANY, _("Zone Edges Orient"), wxDefaultPosition, wxDefaultSize, m_OrientEdgesOptNChoices, m_OrientEdgesOptChoices, 1, wxRA_SPECIFY_COLS );
	m_OrientEdgesOpt->SetSelection( 0 );
	m_OutilinesBoxOpt->Add( m_OrientEdgesOpt, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	wxString m_OutlineAppearanceCtrlChoices[] = { _("Line"), _("Hatched Outline"), _("Full Hatched") };
	int m_OutlineAppearanceCtrlNChoices = sizeof( m_OutlineAppearanceCtrlChoices ) / sizeof( wxString );
	m_OutlineAppearanceCtrl = new wxRadioBox( this, wxID_ANY, _("Outlines Appearence"), wxDefaultPosition, wxDefaultSize, m_OutlineAppearanceCtrlNChoices, m_OutlineAppearanceCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_OutlineAppearanceCtrl->SetSelection( 1 );
	m_OutilinesBoxOpt->Add( m_OutlineAppearanceCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );
	
	
	bSizerRight->Add( m_OutilinesBoxOpt, 0, wxEXPAND|wxALL, 5 );
	
	m_MinThicknessValueTitle = new wxStaticText( this, wxID_ANY, _("Zone min thickness value"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MinThicknessValueTitle->Wrap( -1 );
	bSizerRight->Add( m_MinThicknessValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_ZoneMinThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_ZoneMinThicknessCtrl->SetMaxLength( 0 ); 
	bSizerRight->Add( m_ZoneMinThicknessCtrl, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	
	m_UpperSizer->Add( bSizerRight, 0, wxEXPAND, 5 );
	
	
	m_MainSizer->Add( m_UpperSizer, 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_MainSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_sdbSizerButtons = new wxStdDialogButtonSizer();
	m_sdbSizerButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsOK );
	m_sdbSizerButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerButtons->AddButton( m_sdbSizerButtonsCancel );
	m_sdbSizerButtons->Realize();
	
	m_MainSizer->Add( m_sdbSizerButtons, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE::~DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE()
{
}
