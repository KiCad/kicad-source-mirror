///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_sch_sheet_props_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_SCH_SHEET_PROPS_BASE::DIALOG_SCH_SHEET_PROPS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* mainSizer;
	mainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 2, 6, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("&File name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	fgSizer1->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textFileName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textFileName, 5, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_HORIZONTAL|wxALL|wxEXPAND, 3 );
	
	m_staticText2 = new wxStaticText( this, wxID_ANY, _("Te&xt size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	fgSizer1->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textFileNameSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textFileNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticFileNameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticFileNameSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticFileNameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticText4 = new wxStaticText( this, wxID_ANY, _("&Sheet name:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	fgSizer1->Add( m_staticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textSheetName = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textSheetName, 5, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	
	fgSizer1->Add( 0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 3 );
	
	m_staticText5 = new wxStaticText( this, wxID_ANY, _("&Text size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText5->Wrap( -1 );
	fgSizer1->Add( m_staticText5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_textSheetNameSize = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_textSheetNameSize, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	m_staticSheetNameSizeUnits = new wxStaticText( this, wxID_ANY, _("units"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticSheetNameSizeUnits->Wrap( -1 );
	fgSizer1->Add( m_staticSheetNameSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	mainSizer->Add( fgSizer1, 1, wxALL|wxEXPAND, 12 );
	
	
	mainSizer->Add( 0, 0, 0, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	mainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 12 );
	
	this->SetSizer( mainSizer );
	this->Layout();
	mainSizer->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_SCH_SHEET_PROPS_BASE::~DIALOG_SCH_SHEET_PROPS_BASE()
{
}
