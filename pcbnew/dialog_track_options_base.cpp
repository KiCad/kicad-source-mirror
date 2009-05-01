///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_track_options_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_TRACKS_OPTIONS_BASE, wxDialog )
	EVT_INIT_DIALOG( DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnInitDialog )
	EVT_CHECKBOX( wxID_ANY, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnCheckboxAllowsMicroviaClick )
	EVT_BUTTON( wxID_OK, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonOkClick )
	EVT_BUTTON( wxID_CANCEL, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonCancelClick )
END_EVENT_TABLE()

DIALOG_TRACKS_OPTIONS_BASE::DIALOG_TRACKS_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLeftSizer;
	sbLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias:") ), wxVERTICAL );
	
	m_ViaSizeTitle = new wxStaticText( this, wxID_ANY, _("Via size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaSizeTitle->Wrap( -1 );
	sbLeftSizer->Add( m_ViaSizeTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptViaSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptViaSize->SetToolTip( _("Enter the current via diameter.") );
	
	sbLeftSizer->Add( m_OptViaSize, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_ViaDefaultDrillValueTitle = new wxStaticText( this, wxID_ANY, _("Default Via Drill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaDefaultDrillValueTitle->Wrap( -1 );
	sbLeftSizer->Add( m_ViaDefaultDrillValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptViaDrill = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptViaDrill->SetToolTip( _("Enter the default via drill diameter\nAll vias drills not set to a specific drill value will have this drill value.") );
	
	sbLeftSizer->Add( m_OptViaDrill, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_ViaAltDrillValueTitle = new wxStaticText( this, wxID_ANY, _("Specific Via Drill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaAltDrillValueTitle->Wrap( -1 );
	sbLeftSizer->Add( m_ViaAltDrillValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptCustomViaDrill = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptCustomViaDrill->SetToolTip( _("Use a specific drill value for all vias that must have a given drill value,\nand set the via hole to this specific drill value using the pop up menu.") );
	
	sbLeftSizer->Add( m_OptCustomViaDrill, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	wxString m_OptViaTypeChoices[] = { _("Through Via"), _("Blind or Buried Via") };
	int m_OptViaTypeNChoices = sizeof( m_OptViaTypeChoices ) / sizeof( wxString );
	m_OptViaType = new wxRadioBox( this, wxID_ANY, _("Default Via Type"), wxDefaultPosition, wxDefaultSize, m_OptViaTypeNChoices, m_OptViaTypeChoices, 1, wxRA_SPECIFY_COLS );
	m_OptViaType->SetSelection( 0 );
	m_OptViaType->SetToolTip( _("Select the current via type.\nTrough via is the usual selection") );
	
	sbLeftSizer->Add( m_OptViaType, 0, wxALL, 5 );
	
	bMainSizer->Add( sbLeftSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbMiddleLeftSizer;
	sbMiddleLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Micro Vias:") ), wxVERTICAL );
	
	m_MicroViaSizeTitle = new wxStaticText( this, wxID_ANY, _("Micro Via Size"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaSizeTitle->Wrap( -1 );
	sbMiddleLeftSizer->Add( m_MicroViaSizeTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MicroViaSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbMiddleLeftSizer->Add( m_MicroViaSizeCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_MicroViaDrillTitle = new wxStaticText( this, wxID_ANY, _("Micro Via Drill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MicroViaDrillTitle->Wrap( -1 );
	sbMiddleLeftSizer->Add( m_MicroViaDrillTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MicroViaDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbMiddleLeftSizer->Add( m_MicroViaDrillCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	sbMiddleLeftSizer->Add( 10, 10, 0, 0, 5 );
	
	m_AllowMicroViaCtrl = new wxCheckBox( this, wxID_ANY, _("Allows Micro Vias"), wxDefaultPosition, wxDefaultSize, 0 );
	
	m_AllowMicroViaCtrl->SetToolTip( _("Allows use of micro vias\nThey are very small vias only from an external copper layer to its near neightbour\n") );
	
	sbMiddleLeftSizer->Add( m_AllowMicroViaCtrl, 0, wxALL, 5 );
	
	bMainSizer->Add( sbMiddleLeftSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbMiddleRightSizer;
	sbMiddleRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimensions:") ), wxVERTICAL );
	
	m_TrackWidthTitle = new wxStaticText( this, wxID_ANY, _("Track Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackWidthTitle->Wrap( -1 );
	sbMiddleRightSizer->Add( m_TrackWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptTrackWidth = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptTrackWidth->SetToolTip( _("Enter the current track width") );
	
	sbMiddleRightSizer->Add( m_OptTrackWidth, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_TrackClearanceTitle = new wxStaticText( this, wxID_ANY, _("Clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackClearanceTitle->Wrap( -1 );
	sbMiddleRightSizer->Add( m_TrackClearanceTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptTrackClearance = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptTrackClearance->SetToolTip( _("This is the clearance between tracks, vias and pads for DRC.") );
	
	sbMiddleRightSizer->Add( m_OptTrackClearance, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	
	sbMiddleRightSizer->Add( 10, 10, 0, 0, 5 );
	
	m_MaskClearanceTitle = new wxStaticText( this, wxID_ANY, _("Mask clearance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	sbMiddleRightSizer->Add( m_MaskClearanceTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptMaskMargin = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptMaskMargin->SetToolTip( _("This is the clearance between pads and the mask") );
	
	sbMiddleRightSizer->Add( m_OptMaskMargin, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainSizer->Add( sbMiddleRightSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bRightSizer;
	bRightSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonOK = new wxButton( this, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonOK->SetDefault(); 
	bRightSizer->Add( m_buttonOK, 0, wxALL, 5 );
	
	m_buttonCANCEL = new wxButton( this, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bRightSizer->Add( m_buttonCANCEL, 0, wxALL, 5 );
	
	bMainSizer->Add( bRightSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
}

DIALOG_TRACKS_OPTIONS_BASE::~DIALOG_TRACKS_OPTIONS_BASE()
{
}
