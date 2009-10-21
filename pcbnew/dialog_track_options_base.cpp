///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_track_options_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_TRACKS_OPTIONS_BASE, wxDialog )
	EVT_BUTTON( wxID_ADD_VIA_SIZE, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonAddViaSizeClick )
	EVT_BUTTON( wxID_DELETED_WIA_SIEZ, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonDeleteViaSizeClick )
	EVT_BUTTON( wxID_ADD_TRACK_WIDTH, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonAddTrackSizeClick )
	EVT_BUTTON( wxID_DELETED_TRACK_WIDTH, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonDeleteTrackSizeClick )
	EVT_BUTTON( wxID_CANCEL, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonCancelClick )
	EVT_BUTTON( wxID_OK, DIALOG_TRACKS_OPTIONS_BASE::_wxFB_OnButtonOkClick )
END_EVENT_TABLE()

DIALOG_TRACKS_OPTIONS_BASE::DIALOG_TRACKS_OPTIONS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bMainUpperSizer;
	bMainUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticBoxSizer* sbLeftSizer;
	sbLeftSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias:") ), wxVERTICAL );
	
	wxStaticBoxSizer* sViaSizeBox;
	sViaSizeBox = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Vias Custom Sizes List:") ), wxHORIZONTAL );
	
	m_ViaSizeListCtrl = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	sViaSizeBox->Add( m_ViaSizeListCtrl, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizeViasListButtons;
	bSizeViasListButtons = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddViasSize = new wxButton( this, wxID_ADD_VIA_SIZE, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeViasListButtons->Add( m_buttonAddViasSize, 1, wxALL, 5 );
	
	m_button4 = new wxButton( this, wxID_DELETED_WIA_SIEZ, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizeViasListButtons->Add( m_button4, 1, wxALL, 5 );
	
	sViaSizeBox->Add( bSizeViasListButtons, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sbLeftSizer->Add( sViaSizeBox, 1, wxEXPAND, 5 );
	
	m_ViaAltDrillValueTitle = new wxStaticText( this, wxID_ANY, _("Specific Via Drill"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaAltDrillValueTitle->Wrap( -1 );
	sbLeftSizer->Add( m_ViaAltDrillValueTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptCustomViaDrill = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptCustomViaDrill->SetToolTip( _("Use a specific drill value for all vias that must have a given drill value,\nand set the via hole to this specific drill value using the pop up menu.") );
	
	sbLeftSizer->Add( m_OptCustomViaDrill, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainUpperSizer->Add( sbLeftSizer, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbMiddleRightSizer;
	sbMiddleRightSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Dimensions:") ), wxVERTICAL );
	
	wxStaticBoxSizer* sbTracksListSizer;
	sbTracksListSizer = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Tracks Custom Widths List:") ), wxHORIZONTAL );
	
	m_TrackWidthListCtrl = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	sbTracksListSizer->Add( m_TrackWidthListCtrl, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerTacksButtSizer;
	bSizerTacksButtSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonAddTrackSize = new wxButton( this, wxID_ADD_TRACK_WIDTH, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTacksButtSizer->Add( m_buttonAddTrackSize, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonDeleteTrackWidth = new wxButton( this, wxID_DELETED_TRACK_WIDTH, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerTacksButtSizer->Add( m_buttonDeleteTrackWidth, 0, wxALL|wxEXPAND, 5 );
	
	sbTracksListSizer->Add( bSizerTacksButtSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
	sbMiddleRightSizer->Add( sbTracksListSizer, 1, wxEXPAND, 5 );
	
	
	sbMiddleRightSizer->Add( 10, 10, 0, 0, 5 );
	
	m_MaskClearanceTitle = new wxStaticText( this, wxID_ANY, _("Pads Mask Clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MaskClearanceTitle->Wrap( -1 );
	sbMiddleRightSizer->Add( m_MaskClearanceTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_OptMaskMargin = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_OptMaskMargin->SetToolTip( _("This is the clearance between pads and the mask") );
	
	sbMiddleRightSizer->Add( m_OptMaskMargin, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	bMainUpperSizer->Add( sbMiddleRightSizer, 1, wxEXPAND, 5 );
	
	bMainSizer->Add( bMainUpperSizer, 1, wxEXPAND, 5 );
	
	m_sdbButtonsSizer = new wxStdDialogButtonSizer();
	m_sdbButtonsSizerOK = new wxButton( this, wxID_OK );
	m_sdbButtonsSizer->AddButton( m_sdbButtonsSizerOK );
	m_sdbButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbButtonsSizer->AddButton( m_sdbButtonsSizerCancel );
	m_sdbButtonsSizer->Realize();
	bMainSizer->Add( m_sdbButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	this->SetSizer( bMainSizer );
	this->Layout();
}

DIALOG_TRACKS_OPTIONS_BASE::~DIALOG_TRACKS_OPTIONS_BASE()
{
}
