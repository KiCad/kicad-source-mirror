///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 7, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_filenameLabel = new wxStaticText( this, wxID_ANY, _("&File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filenameLabel->Wrap( -1 );
	fgSizer1->Add( m_filenameLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textFileName->SetMinSize( wxSize( 240,-1 ) );

	fgSizer1->Add( m_textFileName, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxTOP|wxBOTTOM, 5 );

	m_browseButton = new wxBitmapButton( this, ID_BUTTON_BROWSE_SHEET, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_browseButton->SetMinSize( wxSize( 30,28 ) );

	fgSizer1->Add( m_browseButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 2 );


	fgSizer1->Add( 0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxRIGHT|wxLEFT, 10 );

	m_filenameSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filenameSizeLabel->Wrap( -1 );
	fgSizer1->Add( m_filenameSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_filenameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_filenameSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_filenameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_filenameSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_filenameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_sheetnameLabel = new wxStaticText( this, wxID_ANY, _("&Sheet name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sheetnameLabel->Wrap( -1 );
	fgSizer1->Add( m_sheetnameLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_textSheetName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textSheetName, 5, wxEXPAND|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizer1->Add( 0, 0, 0, wxALIGN_CENTER_VERTICAL|wxALL, 10 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sheetnameSizeLabel = new wxStaticText( this, wxID_ANY, _("Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sheetnameSizeLabel->Wrap( -1 );
	fgSizer1->Add( m_sheetnameSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_sheetnameSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_sheetnameSizeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	m_sheetnameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_sheetnameSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_sheetnameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticTextTimeStamp = new wxStaticText( this, wxID_ANY, _("Unique timestamp:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextTimeStamp->Wrap( -1 );
	fgSizer1->Add( m_staticTextTimeStamp, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_textCtrlTimeStamp = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer1->Add( m_textCtrlTimeStamp, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );


	bupperSizer->Add( fgSizer1, 1, wxALL|wxEXPAND, 5 );


	mainSizer->Add( bupperSizer, 0, wxEXPAND|wxTOP, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

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
