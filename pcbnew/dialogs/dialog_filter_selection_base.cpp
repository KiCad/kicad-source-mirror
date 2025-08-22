///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_filter_selection_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FILTER_SELECTION_BASE::DIALOG_FILTER_SELECTION_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 2, 3, 3 );

	m_All_Items = new wxCheckBox( this, wxID_ANY, _("All items"), wxPoint( -1,-1 ), wxDefaultSize, wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER );
	gSizer1->Add( m_All_Items, 0, 0, 5 );

	m_Hidden_Spacer = new wxCheckBox( this, wxID_ANY, wxEmptyString, wxPoint( -1,-1 ), wxDefaultSize, 0 );
	m_Hidden_Spacer->Hide();

	gSizer1->Add( m_Hidden_Spacer, 0, wxRESERVE_SPACE_EVEN_IF_HIDDEN, 0 );

	m_Include_Modules = new wxCheckBox( this, wxID_ANY, _("Include &footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Modules, 0, 0, 5 );

	m_Include_PcbTexts = new wxCheckBox( this, wxID_ANY, _("Include t&ext items"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_PcbTexts, 0, 0, 5 );

	m_IncludeLockedModules = new wxCheckBox( this, wxID_ANY, _("Include &locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_IncludeLockedModules, 0, 0, 5 );

	m_Include_Draw_Items = new wxCheckBox( this, wxID_ANY, _("Include &drawings"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Draw_Items, 0, 0, 5 );

	m_Include_Tracks = new wxCheckBox( this, wxID_ANY, _("Include &tracks"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Tracks, 0, 0, 5 );

	m_Include_Edges_Items = new wxCheckBox( this, wxID_ANY, _("Include &board outline layer"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Edges_Items, 0, 0, 5 );

	m_Include_Vias = new wxCheckBox( this, wxID_ANY, _("Include &vias"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Vias, 0, 0, 5 );

	m_Include_Zones = new wxCheckBox( this, wxID_ANY, _("Include &zones"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_Include_Zones, 0, 0, 5 );


	bSizerMain->Add( gSizer1, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizerMain->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_All_Items->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::allItemsClicked ), NULL, this );
	m_Include_Modules->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_PcbTexts->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_IncludeLockedModules->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Draw_Items->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Tracks->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Edges_Items->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Vias->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Zones->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::ExecuteCommand ), NULL, this );
}

DIALOG_FILTER_SELECTION_BASE::~DIALOG_FILTER_SELECTION_BASE()
{
	// Disconnect Events
	m_All_Items->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::allItemsClicked ), NULL, this );
	m_Include_Modules->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_PcbTexts->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_IncludeLockedModules->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Draw_Items->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Tracks->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Edges_Items->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Vias->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_Include_Zones->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::checkBoxClicked ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FILTER_SELECTION_BASE::ExecuteCommand ), NULL, this );

}
