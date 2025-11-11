///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "dialog_import_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORT_SETTINGS_BASE::DIALOG_IMPORT_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	m_MainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText* importFromLabel;
	importFromLabel = new wxStaticText( this, wxID_ANY, _("Import from:"), wxDefaultPosition, wxDefaultSize, 0 );
	importFromLabel->Wrap( -1 );
	bUpperSizer->Add( importFromLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_filePathCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_filePathCtrl->SetToolTip( _("Target directory for plot files. Can be absolute or relative to the board file location.") );
	m_filePathCtrl->SetMinSize( wxSize( 300,-1 ) );

	bUpperSizer->Add( m_filePathCtrl, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_browseButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), wxBU_AUTODRAW|0 );
	bUpperSizer->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_MainSizer->Add( bUpperSizer, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bMiddleSizer;
	bMiddleSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftCol;
	bLeftCol = new wxBoxSizer( wxVERTICAL );

	wxStaticText* importLabel;
	importLabel = new wxStaticText( this, wxID_ANY, _("Import:"), wxDefaultPosition, wxDefaultSize, 0 );
	importLabel->Wrap( -1 );
	bLeftCol->Add( importLabel, 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_LayersOpt = new wxCheckBox( this, wxID_ANY, _("Board layers and physical stackup"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_LayersOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_MaskAndPasteOpt = new wxCheckBox( this, wxID_ANY, _("Solder mask/paste defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_MaskAndPasteOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ZoneHatchingOffsetsOpt = new wxCheckBox( this, wxID_ANY, _("Zone hatched fill offsets"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_ZoneHatchingOffsetsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TextAndGraphicsOpt = new wxCheckBox( this, wxID_ANY, _("Text && graphics default properties"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_TextAndGraphicsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_FormattingOpt = new wxCheckBox( this, wxID_ANY, _("Text && graphics formatting"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_FormattingOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ConstraintsOpt = new wxCheckBox( this, wxID_ANY, _("Design rule constraints"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_ConstraintsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TracksAndViasOpt = new wxCheckBox( this, wxID_ANY, _("Predefined track && via dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_TracksAndViasOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TeardropsOpt = new wxCheckBox( this, wxID_ANY, _("Teardrop defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_TeardropsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_TuningPatternsOpt = new wxCheckBox( this, wxID_ANY, _("Length-tuning pattern defaults"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_TuningPatternsOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_NetclassesOpt = new wxCheckBox( this, wxID_ANY, _("Net classes"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_NetclassesOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ComponentClassesOpt = new wxCheckBox( this, wxID_ANY, _("Component classes"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_ComponentClassesOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_TuningProfilesOpt = new wxCheckBox( this, wxID_ANY, _("Tuning Profiles"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_TuningProfilesOpt, 0, wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	m_CustomRulesOpt = new wxCheckBox( this, wxID_ANY, _("Custom rules"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_CustomRulesOpt, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_SeveritiesOpt = new wxCheckBox( this, wxID_ANY, _("Violation severities"), wxDefaultPosition, wxDefaultSize, 0 );
	bLeftCol->Add( m_SeveritiesOpt, 0, wxRIGHT|wxLEFT, 5 );


	bMiddleSizer->Add( bLeftCol, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );


	m_MainSizer->Add( bMiddleSizer, 1, wxEXPAND|wxBOTTOM, 5 );

	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_selectAllButton = new wxButton( this, wxID_ANY, _("Select All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_buttonsSizer->Add( m_selectAllButton, 0, wxALIGN_CENTER|wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL, 10 );

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
	m_LayersOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_MaskAndPasteOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TextAndGraphicsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_ConstraintsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TracksAndViasOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TeardropsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TuningPatternsOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_NetclassesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_ComponentClassesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TuningProfilesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_CustomRulesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_SeveritiesOpt->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_selectAllButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnSelectAll ), NULL, this );
}

DIALOG_IMPORT_SETTINGS_BASE::~DIALOG_IMPORT_SETTINGS_BASE()
{
	// Disconnect Events
	m_browseButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnBrowseClicked ), NULL, this );
	m_LayersOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_MaskAndPasteOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TextAndGraphicsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_ConstraintsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TracksAndViasOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TeardropsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TuningPatternsOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_NetclassesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_ComponentClassesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_TuningProfilesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_CustomRulesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_SeveritiesOpt->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnCheckboxClicked ), NULL, this );
	m_selectAllButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORT_SETTINGS_BASE::OnSelectAll ), NULL, this );

}
