///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sch_sheet_props_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_SHEET_PROPS_BASE::DIALOG_SCH_SHEET_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,150 ), wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 6, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("&File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textFileName->SetMaxLength( 0 ); 
	m_textFileName->SetMinSize( wxSize( 200,-1 ) );
	
	fgSizer1->Add( m_textFileName, 5, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 3 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Si&ze:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textFileNameSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textFileNameSize->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textFileNameSize, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 3 );
	
	m_staticFileNameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticFileNameSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticFileNameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("&Sheet name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer1->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textSheetName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textSheetName->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textSheetName, 5, wxEXPAND|wxTOP|wxBOTTOM, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("&Size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textSheetNameSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textSheetNameSize->SetMaxLength( 0 ); 
	fgSizer1->Add( m_textSheetNameSize, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 3 );
	
	m_staticSheetNameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticSheetNameSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticSheetNameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline2, 0, wxEXPAND|wxALL, 5 );
	
	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline3, 0, wxEXPAND | wxALL, 5 );
	
	
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
	
	
	mainSizer->Add( bupperSizer, 0, wxEXPAND, 5 );
	
	
	mainSizer->Add( 0, 0, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	mainSizer->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	mainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	this->SetSizer( mainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
}

DIALOG_SCH_SHEET_PROPS_BASE::~DIALOG_SCH_SHEET_PROPS_BASE()
{
}
