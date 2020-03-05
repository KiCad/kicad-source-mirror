///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sch_sheet_props_base.h"

///////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE( DIALOG_SCH_SHEET_PROPS_BASE, DIALOG_SHIM )
	EVT_BUTTON( ID_BUTTON_BROWSE_SHEET, DIALOG_SCH_SHEET_PROPS_BASE::_wxFB_OnBrowseClicked )
END_EVENT_TABLE()

DIALOG_SCH_SHEET_PROPS_BASE::DIALOG_SCH_SHEET_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,150 ), wxDefaultSize );

	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_filenameLabel = new wxStaticText( this, wxID_ANY, _("&File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filenameLabel->Wrap( -1 );
	bSizer3->Add( m_filenameLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textFileName->SetMinSize( wxSize( 360,-1 ) );

	bSizer3->Add( m_textFileName, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_browseButton = new wxBitmapButton( this, ID_BUTTON_BROWSE_SHEET, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_browseButton->SetMinSize( wxSize( 30,28 ) );

	bSizer3->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bupperSizer->Add( bSizer3, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerFilenameSettings;
	bSizerFilenameSettings = new wxBoxSizer( wxHORIZONTAL );

	m_filenameVisible = new wxCheckBox( this, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFilenameSettings->Add( m_filenameVisible, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 36 );

	m_filenameSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filenameSizeLabel->Wrap( -1 );
	bSizerFilenameSettings->Add( m_filenameSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_filenameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerFilenameSettings->Add( m_filenameSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_filenameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filenameSizeUnits->Wrap( -1 );
	bSizerFilenameSettings->Add( m_filenameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bupperSizer->Add( bSizerFilenameSettings, 0, wxEXPAND, 5 );


	bupperSizer->Add( 0, 15, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_sheetnameLabel = new wxStaticText( this, wxID_ANY, _("&Sheet name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sheetnameLabel->Wrap( -1 );
	bSizer5->Add( m_sheetnameLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textSheetName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_textSheetName, 5, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );


	bupperSizer->Add( bSizer5, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizerSheetnameSettings;
	bSizerSheetnameSettings = new wxBoxSizer( wxHORIZONTAL );

	m_sheetnameVisible = new wxCheckBox( this, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerSheetnameSettings->Add( m_sheetnameVisible, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 36 );

	m_sheetnameSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sheetnameSizeLabel->Wrap( -1 );
	bSizerSheetnameSettings->Add( m_sheetnameSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_sheetnameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerSheetnameSettings->Add( m_sheetnameSizeCtrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_sheetnameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sheetnameSizeUnits->Wrap( -1 );
	bSizerSheetnameSettings->Add( m_sheetnameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bupperSizer->Add( bSizerSheetnameSettings, 0, wxEXPAND, 5 );


	bupperSizer->Add( 0, 20, 0, 0, 5 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bupperSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );

	m_staticTextUuid = new wxStaticText( this, wxID_ANY, _("UUID:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextUuid->Wrap( -1 );
	bupperSizer->Add( m_staticTextUuid, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textCtrlUuid = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bupperSizer->Add( m_textCtrlUuid, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticTextHpath = new wxStaticText( this, wxID_ANY, _("Hierarchical Path:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextHpath->Wrap( -1 );
	bupperSizer->Add( m_staticTextHpath, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	m_textCtrlHpath = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	bupperSizer->Add( m_textCtrlHpath, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	mainSizer->Add( bupperSizer, 0, wxEXPAND|wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	mainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_SCH_SHEET_PROPS_BASE::~DIALOG_SCH_SHEET_PROPS_BASE()
{
}
