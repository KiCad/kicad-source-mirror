///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_import_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_SETTINGS_BASE::DIALOG_IMPORT_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	m_MainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxStaticText* importFromLabel;
	importFromLabel = new wxStaticText( this, wxID_ANY, _("Import from:"), wxDefaultPosition, wxDefaultSize, 0 );
	importFromLabel->Wrap( -1 );
	bupperSizer->Add( importFromLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_filePathCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_filePathCtrl->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_filePathCtrl->SetMinSize( wxSize( 300,-1 ) );
	
	bupperSizer->Add( m_filePathCtrl, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );
	
	m_browseButton = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW );
	m_browseButton->SetMinSize( wxSize( 29,29 ) );
	
	bupperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );
	
	
	m_MainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bmiddleSizer;
	bmiddleSizer = new wxBoxSizer( wxVERTICAL );
	
	wxStaticText* importLabel;
	importLabel = new wxStaticText( this, wxID_ANY, _("Import:"), wxDefaultPosition, wxDefaultSize, 0 );
	importLabel->Wrap( -1 );
	bmiddleSizer->Add( importLabel, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_LayersOpt = new wxCheckBox( this, wxID_ANY, _("Layers setup"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_LayersOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TextAndGraphicsOpt = new wxCheckBox( this, wxID_ANY, _("Text && Graphics default properties"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_TextAndGraphicsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_ConstraintsOpt = new wxCheckBox( this, wxID_ANY, _("Design Rules"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_ConstraintsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_NetclassesOpt = new wxCheckBox( this, wxID_ANY, _("Net Classes"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_NetclassesOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_TracksAndViasOpt = new wxCheckBox( this, wxID_ANY, _("Predefined Track and Via dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_TracksAndViasOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_MaskAndPasteOpt = new wxCheckBox( this, wxID_ANY, _("Solder Mask/Paste defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_MaskAndPasteOpt, 0, wxRIGHT|wxLEFT, 5 );
	
	
	m_MainSizer->Add( bmiddleSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );
	
	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_selectAllButton = new wxButton( this, wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_selectAllButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	m_buttonsSizer->Add( m_sdbSizer1, 1, wxALL|wxEXPAND, 5 );
	
	
	m_MainSizer->Add( m_buttonsSizer, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnBrowseClicked ), NULL, this );
	m_selectAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnSelectAll ), NULL, this );
}

DIALOG_IMPORT_SETTINGS_BASE::~DIALOG_IMPORT_SETTINGS_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnBrowseClicked ), NULL, this );
	m_selectAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnSelectAll ), NULL, this );
	
}
