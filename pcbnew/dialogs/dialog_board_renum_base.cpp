///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 21 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_board_renum_base.h"

 ///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_RENUM_BASE::DIALOG_BOARD_RENUM_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 6, 4, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_FrontSortDirText = new wxStaticText( this, wxID_ANY, _("Annotation Direction"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontSortDirText->Wrap( -1 );
	m_FrontSortDirText->SetToolTip( _("Set the direction of sort: Top to Bottom, left to right is as you read a book") );

	fgSizer1->Add( m_FrontSortDirText, 0, wxALL, 5 );

	wxString m_SortDirChoices[] = { _("Top to Bottom, Left to Right"), _("Top to Bottom, Right to Left"), _("Bottom to Top, Left to Right"), _("Bottom to Top, Right to Left"), _("Left to Right, Top to Bottom"), _("Left to Right, Bottom to Top"), _("Right to Left, Top to Bottom"), _("Right to Left,  Bottom to Top") };
	int m_SortDirNChoices = sizeof( m_SortDirChoices ) / sizeof( wxString );
	m_SortDir = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_SortDirNChoices, m_SortDirChoices, 0 );
	m_SortDir->SetSelection( 0 );
	m_SortDir->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT ) );
	m_SortDir->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );

	fgSizer1->Add( m_SortDir, 0, wxALL, 5 );

	m_Dummy = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Dummy->Wrap( -1 );
	fgSizer1->Add( m_Dummy, 0, wxALL, 5 );

	m_staticText15 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText15->Wrap( -1 );
	fgSizer1->Add( m_staticText15, 0, wxALL, 5 );

	m_FrontRefDesStartText = new wxStaticText( this, wxID_ANY, _("Front Ref Des Start"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontRefDesStartText->Wrap( -1 );
	m_FrontRefDesStartText->SetToolTip( _("Default is 1") );

	fgSizer1->Add( m_FrontRefDesStartText, 0, wxALL, 5 );

	m_FrontRefDesStart = new wxTextCtrl( this, wxID_ANY, _("1"), wxDefaultPosition, wxSize( 100,-1 ), 0 );
	fgSizer1->Add( m_FrontRefDesStart, 0, wxALL, 5 );

	m_BottomRefDesStartText = new wxStaticText( this, wxID_ANY, _("Back Ref Des Start\n(Blank continues from Front)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BottomRefDesStartText->Wrap( -1 );
	m_BottomRefDesStartText->SetToolTip( _("Leave blank or zero, or enter a number greater than the highest reference designation on the front.") );

	fgSizer1->Add( m_BottomRefDesStartText, 0, wxALL, 5 );

	m_BackRefDesStart = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 100,-1 ), 0 );
	fgSizer1->Add( m_BackRefDesStart, 0, wxALL, 5 );

	m_FrontPrefixText = new wxStaticText( this, wxID_ANY, _("Front Prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontPrefixText->Wrap( -1 );
	m_FrontPrefixText->SetToolTip( _("Optional prefix for component side reference designations (i.e. F_)") );

	fgSizer1->Add( m_FrontPrefixText, 0, wxALL, 5 );

	m_FrontPrefix = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	fgSizer1->Add( m_FrontPrefix, 0, wxALL, 1 );

	m_BackPrefixText = new wxStaticText( this, wxID_ANY, _("Back Prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BackPrefixText->Wrap( -1 );
	m_BackPrefixText->SetToolTip( _("Optional prefix for solder side reference designations (i.e. B_)") );

	fgSizer1->Add( m_BackPrefixText, 0, wxALL, 5 );

	m_BackPrefix = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	fgSizer1->Add( m_BackPrefix, 0, wxALL, 1 );

	m_RemoveFrontPrefix = new wxCheckBox( this, wxID_ANY, _("Remove Front Prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RemoveFrontPrefix->SetToolTip( _("If checked will remove the component side prefix") );

	fgSizer1->Add( m_RemoveFrontPrefix, 0, wxALL, 5 );

	m_staticText31 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	fgSizer1->Add( m_staticText31, 0, wxALL, 5 );

	m_RemoveBackPrefix = new wxCheckBox( this, wxID_ANY, _("Remove Back Prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RemoveBackPrefix->SetToolTip( _("If checked will remove the Bottom side prefix") );

	fgSizer1->Add( m_RemoveBackPrefix, 0, wxALL, 5 );

	m_staticText35 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText35->Wrap( -1 );
	fgSizer1->Add( m_staticText35, 0, wxALL, 5 );

	m_SortGridText = new wxStaticText( this, wxID_ANY, _("Sort Grid"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SortGridText->Wrap( -1 );
	m_SortGridText->SetToolTip( _("Sets grid rounding for position") );

	fgSizer1->Add( m_SortGridText, 0, wxALL, 5 );

	m_SortGrid = new wxTextCtrl( this, wxID_ANY, _("1.000"), wxDefaultPosition, wxSize( 200,-1 ), 0 );
	fgSizer1->Add( m_SortGrid, 0, wxALL, 10 );

	m_SortOnModules = new wxRadioButton( this, wxID_ANY, _("Sort on Module Coordinates"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SortOnModules->SetValue( true );
	m_SortOnModules->SetToolTip( _("Sort on the origin of the module") );

	fgSizer1->Add( m_SortOnModules, 0, wxALL, 10 );

	m_SortOnRefDes = new wxRadioButton( this, wxID_ANY, _("Sort on Ref Des Coordinates"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SortOnRefDes->SetToolTip( _("Sort on the origin of the reference designation of the module") );

	fgSizer1->Add( m_SortOnRefDes, 0, wxALL, 10 );

	m_ExcludeListText = new wxStaticText( this, wxID_ANY, _("Exclude Ref Des"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExcludeListText->Wrap( -1 );
	m_ExcludeListText->SetToolTip( _("Do not re-annotate this type \nof Ref Des (R means R*)") );

	fgSizer1->Add( m_ExcludeListText, 0, wxALL, 5 );

	m_ExcludeList = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0 );
	fgSizer1->Add( m_ExcludeList, 0, wxALL, 5 );

	m_WriteChangeFile = new wxCheckBox( this, wxID_ANY, _("Write change file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_WriteChangeFile->SetToolTip( _("Write a \"Was/Is\" file with format Was->Is") );

	fgSizer1->Add( m_WriteChangeFile, 0, wxALL, 5 );

	m_WriteLogFile = new wxCheckBox( this, wxID_ANY, _("Generate log file"), wxDefaultPosition, wxDefaultSize, 0 );
	m_WriteLogFile->SetToolTip( _("Generate a log file (useful for debugging)") );

	fgSizer1->Add( m_WriteLogFile, 0, wxALL, 5 );


	bMainSizer->Add( fgSizer1, 1, wxEXPAND, 5 );

	wxBoxSizer* bLowerSizer;
	bLowerSizer = new wxBoxSizer( wxVERTICAL );

	bLowerSizer->SetMinSize( wxSize( 660,250 ) );
	m_MessageWindow = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_RICH|wxTE_RICH2 );
	bLowerSizer->Add( m_MessageWindow, 1, wxALIGN_TOP|wxALL|wxEXPAND, 5 );


	bMainSizer->Add( bLowerSizer, 1, wxEXPAND, 1 );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 4, 0, 0 );

	m_RenumberButton = new wxButton( this, wxID_ANY, _("Renumber"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RenumberButton->SetToolTip( _("Renumber the board and back-annotate the schematics") );

	gSizer1->Add( m_RenumberButton, 0, wxALL|wxEXPAND, 5 );

	m_staticText38 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText38->Wrap( -1 );
	gSizer1->Add( m_staticText38, 0, wxALL, 5 );

	m_staticText381 = new wxStaticText( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText381->Wrap( -1 );
	gSizer1->Add( m_staticText381, 0, wxALL, 5 );

	m_ExitButton = new wxButton( this, wxID_ANY, _("Exit"), wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_ExitButton, 0, wxALL|wxEXPAND, 5 );


	bSizer9->Add( gSizer1, 1, wxEXPAND, 5 );


	bMainSizer->Add( bSizer9, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_RenumberButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_RENUM_BASE::OnRenumberClick ), NULL, this );
	m_ExitButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_RENUM_BASE::OKDone ), NULL, this );
}

DIALOG_BOARD_RENUM_BASE::~DIALOG_BOARD_RENUM_BASE()
{
	// Disconnect Events
	m_RenumberButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_RENUM_BASE::OnRenumberClick ), NULL, this );
	m_ExitButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_RENUM_BASE::OKDone ), NULL, this );

}
