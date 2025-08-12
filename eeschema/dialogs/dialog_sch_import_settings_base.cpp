///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_sch_import_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_IMPORT_SETTINGS_BASE::DIALOG_SCH_IMPORT_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* importFromLabel;
	importFromLabel = new wxStaticText( this, wxID_ANY, _("Import from:"), wxDefaultPosition, wxDefaultSize, 0 );
	importFromLabel->Wrap( -1 );
	bupperSizer->Add( importFromLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_filePathCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_filePathCtrl->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_filePathCtrl->SetMinSize( wxSize( 300,-1 ) );

	bupperSizer->Add( m_filePathCtrl, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bupperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_MainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bmiddleSizer;
	bmiddleSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticText* importLabel;
	importLabel = new wxStaticText( this, wxID_ANY, _("Import:"), wxDefaultPosition, wxDefaultSize, 0 );
	importLabel->Wrap( -1 );
	bmiddleSizer->Add( importLabel, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_FormattingOpt = new wxCheckBox( this, wxID_ANY, _("Formatting preferences"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_FormattingOpt, 0, wxALL, 5 );

	m_annotationOpt = new wxCheckBox( this, wxID_ANY, _("Annotation preferences"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_annotationOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_FieldNameTemplatesOpt = new wxCheckBox( this, wxID_ANY, _("Field name templates"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_FieldNameTemplatesOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_BomPresetsOpt = new wxCheckBox( this, wxID_ANY, _("BOM presets"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_BomPresetsOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_BomFmtPresetsOpt = new wxCheckBox( this, wxID_ANY, _("BOM format presets"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_BomFmtPresetsOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_SeveritiesOpt = new wxCheckBox( this, wxID_ANY, _("Violation severities"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_SeveritiesOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_PinMapOpt = new wxCheckBox( this, wxID_ANY, _("Pin conflict map"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_PinMapOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_NetClassesOpt = new wxCheckBox( this, wxID_ANY, _("Net classes"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_NetClassesOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_BusAliasesOpt = new wxCheckBox( this, wxID_ANY, _("Bus alias definitions"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_BusAliasesOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextVarsOpt = new wxCheckBox( this, wxID_ANY, _("Text variables"), wxDefaultPosition, wxDefaultSize, 0 );
	bmiddleSizer->Add( m_TextVarsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


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

	m_buttonsSizer->Add( m_sdbSizer1, 1, wxBOTTOM|wxEXPAND|wxTOP, 5 );


	m_MainSizer->Add( m_buttonsSizer, 0, wxEXPAND, 5 );


	this->SetSizer( m_MainSizer );
	this->Layout();
	m_MainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_browseButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_IMPORT_SETTINGS_BASE::OnBrowseClicked ), NULL, this );
	m_selectAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_IMPORT_SETTINGS_BASE::OnSelectAll ), NULL, this );
}

DIALOG_SCH_IMPORT_SETTINGS_BASE::~DIALOG_SCH_IMPORT_SETTINGS_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_IMPORT_SETTINGS_BASE::OnBrowseClicked ), NULL, this );
	m_selectAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_SCH_IMPORT_SETTINGS_BASE::OnSelectAll ), NULL, this );

}
