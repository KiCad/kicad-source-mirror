///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_grid.h"

#include "dialog_executecommand_job_settings_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 5, 5 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableRow( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_textCommand = new wxStaticText( this, wxID_ANY, _("Command:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textCommand->Wrap( -1 );
	fgSizer1->Add( m_textCommand, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_textCtrlCommand = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_textCtrlCommand->SetUseTabs( false );
	m_textCtrlCommand->SetTabWidth( 4 );
	m_textCtrlCommand->SetIndent( 4 );
	m_textCtrlCommand->SetTabIndents( false );
	m_textCtrlCommand->SetBackSpaceUnIndents( false );
	m_textCtrlCommand->SetViewEOL( false );
	m_textCtrlCommand->SetViewWhiteSpace( false );
	m_textCtrlCommand->SetMarginWidth( 2, 0 );
	m_textCtrlCommand->SetIndentationGuides( false );
	m_textCtrlCommand->SetReadOnly( false );
	m_textCtrlCommand->SetMarginWidth( 1, 0 );
	m_textCtrlCommand->SetMarginWidth( 0, 0 );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_textCtrlCommand->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_textCtrlCommand->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_textCtrlCommand->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_textCtrlCommand->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_textCtrlCommand->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_textCtrlCommand->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_textCtrlCommand->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_textCtrlCommand->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_textCtrlCommand->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_textCtrlCommand->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_textCtrlCommand->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	fgSizer1->Add( m_textCtrlCommand, 1, wxEXPAND|wxTOP|wxBOTTOM|wxRIGHT, 5 );

	m_textOutputPath = new wxStaticText( this, wxID_ANY, _("Output path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textOutputPath->Wrap( -1 );
	fgSizer1->Add( m_textOutputPath, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_textCtrlOutputPath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textCtrlOutputPath->SetMinSize( wxSize( 350,-1 ) );

	fgSizer1->Add( m_textCtrlOutputPath, 1, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );


	bSizerMain->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxVERTICAL );

	m_cbRecordOutput = new wxCheckBox( this, wxID_ANY, _("Record output messages"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbRecordOutput, 0, wxLEFT, 5 );

	m_cbIgnoreExitCode = new wxCheckBox( this, wxID_ANY, _("Ignore non-zero exit code"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_cbIgnoreExitCode, 0, wxLEFT|wxTOP, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( 0, 10, 0, wxEXPAND, 5 );

	wxStaticText* stPathsLabel;
	stPathsLabel = new wxStaticText( this, wxID_ANY, _("Available text variables:"), wxDefaultPosition, wxDefaultSize, 0 );
	stPathsLabel->Wrap( -1 );
	bSizerMain->Add( stPathsLabel, 0, wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerMain->Add( 0, 2, 0, wxEXPAND, 5 );

	m_path_subs_grid = new WX_GRID( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_path_subs_grid->CreateGrid( 1, 2 );
	m_path_subs_grid->EnableEditing( false );
	m_path_subs_grid->EnableGridLines( true );
	m_path_subs_grid->EnableDragGridSize( false );
	m_path_subs_grid->SetMargins( 0, 0 );

	// Columns
	m_path_subs_grid->SetColSize( 0, 200 );
	m_path_subs_grid->SetColSize( 1, 300 );
	m_path_subs_grid->AutoSizeColumns();
	m_path_subs_grid->EnableDragColMove( false );
	m_path_subs_grid->EnableDragColSize( true );
	m_path_subs_grid->SetColLabelSize( 0 );
	m_path_subs_grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_path_subs_grid->EnableDragRowSize( true );
	m_path_subs_grid->SetRowLabelSize( 0 );
	m_path_subs_grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_path_subs_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
	m_path_subs_grid->SetToolTip( _("This is a read-only table which shows pertinent environment variables.") );

	bSizerMain->Add( m_path_subs_grid, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_cbRecordOutput->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::OnRecordOutputClicked ), NULL, this );
	m_path_subs_grid->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::onSizeGrid ), NULL, this );
}

DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::~DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE()
{
	// Disconnect Events
	m_cbRecordOutput->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::OnRecordOutputClicked ), NULL, this );
	m_path_subs_grid->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE::onSizeGrid ), NULL, this );

}
