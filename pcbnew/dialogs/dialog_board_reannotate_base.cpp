///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_panel.h"

#include "dialog_board_reannotate_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_REANNOTATE_BASE::DIALOG_BOARD_REANNOTATE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerOpts;
	bSizerOpts = new wxBoxSizer( wxVERTICAL );

	wxStaticText* stOrderLabel;
	stOrderLabel = new wxStaticText( this, wxID_ANY, _("Footprint Order"), wxDefaultPosition, wxDefaultSize, 0 );
	stOrderLabel->Wrap( -1 );
	bSizerOpts->Add( stOrderLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerOpts->Add( m_staticline1, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizerButtons;
	fgSizerButtons = new wxFlexGridSizer( 2, 23, 0, 0 );
	fgSizerButtons->AddGrowableCol( 2 );
	fgSizerButtons->AddGrowableCol( 5 );
	fgSizerButtons->AddGrowableCol( 8 );
	fgSizerButtons->AddGrowableCol( 11 );
	fgSizerButtons->AddGrowableCol( 14 );
	fgSizerButtons->AddGrowableCol( 17 );
	fgSizerButtons->AddGrowableCol( 20 );
	fgSizerButtons->SetFlexibleDirection( wxBOTH );
	fgSizerButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_Down_Right = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_Down_Right->SetValue( true );
	fgSizerButtons->Add( m_Down_Right, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5 );

	reannotate_down_right_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_down_right_bitmap->SetToolTip( _("Horizontally: top left to bottom right") );

	fgSizerButtons->Add( reannotate_down_right_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Right_Down = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Right_Down, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_right_down_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_right_down_bitmap->SetToolTip( _("Horizontally: top right to bottom left") );

	fgSizerButtons->Add( reannotate_right_down_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Down_Left = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Down_Left, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_down_left_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_down_left_bitmap->SetToolTip( _("Horizontally: bottom left to top right") );

	fgSizerButtons->Add( reannotate_down_left_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Left_Down = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Left_Down, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_left_down_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_left_down_bitmap->SetToolTip( _("Horizontally:: bottom right to top left") );

	fgSizerButtons->Add( reannotate_left_down_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Up_Right = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Up_Right, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_up_right_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_up_right_bitmap->SetToolTip( _("Vertically: top left to bottom right") );

	fgSizerButtons->Add( reannotate_up_right_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Right_Up = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Right_Up, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_right_up_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_right_up_bitmap->SetToolTip( _("Vertically: bottom left to top right") );

	fgSizerButtons->Add( reannotate_right_up_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Up_Left = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Up_Left, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_up_left_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_up_left_bitmap->SetToolTip( _("Vertically: top right to bottom left") );

	fgSizerButtons->Add( reannotate_up_left_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	fgSizerButtons->Add( 10, 0, 1, wxEXPAND, 5 );

	m_Left_Up = new wxRadioButton( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Left_Up, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	reannotate_left_up_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( -1,-1 ), 0 );
	reannotate_left_up_bitmap->SetToolTip( _("Vertically: bottom right to top left") );

	fgSizerButtons->Add( reannotate_left_up_bitmap, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerOpts->Add( fgSizerButtons, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );

	wxFlexGridSizer* fgSizerLocations;
	fgSizerLocations = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizerLocations->SetFlexibleDirection( wxBOTH );
	fgSizerLocations->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText9 = new wxStaticText( this, wxID_ANY, _("Based on location of:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizerLocations->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxString m_locationChoiceChoices[] = { _("Footprint"), _("Reference") };
	int m_locationChoiceNChoices = sizeof( m_locationChoiceChoices ) / sizeof( wxString );
	m_locationChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_locationChoiceNChoices, m_locationChoiceChoices, 0 );
	m_locationChoice->SetSelection( 0 );
	fgSizerLocations->Add( m_locationChoice, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_SortGridText = new wxStaticText( this, wxID_ANY, _("Round locations to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SortGridText->Wrap( -1 );
	m_SortGridText->SetToolTip( _("Component position will be rounded\nto this grid before sorting.\nThis helps with misaligned parts.") );

	fgSizerLocations->Add( m_SortGridText, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 25 );

	wxArrayString m_GridChoiceChoices;
	m_GridChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_GridChoiceChoices, 0 );
	m_GridChoice->SetSelection( 0 );
	m_GridChoice->SetToolTip( _("Component position will be rounded\nto this grid before sorting.\nThis helps with misaligned parts.") );

	fgSizerLocations->Add( m_GridChoice, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	bSizerOpts->Add( fgSizerLocations, 0, wxEXPAND|wxALL, 10 );

	wxStaticText* stScopeLabel;
	stScopeLabel = new wxStaticText( this, wxID_ANY, _("Reannotation Scope"), wxDefaultPosition, wxDefaultSize, 0 );
	stScopeLabel->Wrap( -1 );
	bSizerOpts->Add( stScopeLabel, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerOpts->Add( m_staticline2, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxFlexGridSizer* fgSizer6111;
	fgSizer6111 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer6111->SetFlexibleDirection( wxVERTICAL );
	fgSizer6111->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	m_AnnotateAll = new wxRadioButton( this, wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_AnnotateAll->SetValue( true );
	fgSizer6111->Add( m_AnnotateAll, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_AnnotateFront = new wxRadioButton( this, wxID_ANY, _("Front"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6111->Add( m_AnnotateFront, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_AnnotateBack = new wxRadioButton( this, wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6111->Add( m_AnnotateBack, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );

	m_AnnotateSelection = new wxRadioButton( this, wxID_ANY, _("Selection"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6111->Add( m_AnnotateSelection, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerOpts->Add( fgSizer6111, 0, wxBOTTOM|wxLEFT, 5 );

	m_ExcludeLocked = new wxCheckBox( this, wxID_ANY, _("Exclude locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExcludeLocked->SetToolTip( _("Locked footprints will not be reannotated") );

	bSizerOpts->Add( m_ExcludeLocked, 0, wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* bSizerExclusions;
	bSizerExclusions = new wxBoxSizer( wxHORIZONTAL );

	m_ExcludeListText = new wxStaticText( this, wxID_ANY, _("Exclude references:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExcludeListText->Wrap( -1 );
	m_ExcludeListText->SetToolTip( _("Do not re-annotate this type \nof reference (R means R*)") );

	bSizerExclusions->Add( m_ExcludeListText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_ExcludeList = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerExclusions->Add( m_ExcludeList, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerOpts->Add( bSizerExclusions, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_staticText10 = new wxStaticText( this, wxID_ANY, _("Reference Designators"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText10->Wrap( -1 );
	bSizerOpts->Add( m_staticText10, 0, wxTOP|wxRIGHT|wxLEFT, 13 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerOpts->Add( m_staticline3, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 40,10 ) );

	m_FrontRefDesStartText = new wxStaticText( this, wxID_ANY, _("Front reference start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontRefDesStartText->Wrap( -1 );
	m_FrontRefDesStartText->SetToolTip( _("Starting reference designation for front.") );

	gbSizer1->Add( m_FrontRefDesStartText, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_FrontRefDesStart = new wxTextCtrl( this, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontRefDesStart->SetToolTip( _("Default is 1") );
	m_FrontRefDesStart->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_FrontRefDesStart, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_BottomRefDesStartText = new wxStaticText( this, wxID_ANY, _("Back reference start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BottomRefDesStartText->Wrap( -1 );
	m_BottomRefDesStartText->SetToolTip( _("Blank continues from front or enter a number greater than the highest reference designation on the front.") );

	gbSizer1->Add( m_BottomRefDesStartText, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_BackRefDesStart = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_BackRefDesStart->SetToolTip( _("Leave blank or zero, or enter a number greater than the highest reference designation on the front.") );
	m_BackRefDesStart->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_BackRefDesStart, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_FrontPrefixText = new wxStaticText( this, wxID_ANY, _("Front prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontPrefixText->Wrap( -1 );
	m_FrontPrefixText->SetToolTip( _("Optional prefix for component side reference designations (e.g. F_)") );

	gbSizer1->Add( m_FrontPrefixText, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_FrontPrefix = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontPrefix->SetToolTip( _("Optional prefix for component side reference designations (e.g. F_)") );

	gbSizer1->Add( m_FrontPrefix, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_BackPrefixText = new wxStaticText( this, wxID_ANY, _("Back prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BackPrefixText->Wrap( -1 );
	m_BackPrefixText->SetToolTip( _("Optional prefix for solder side reference designations (e.g. B_)") );

	gbSizer1->Add( m_BackPrefixText, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_BackPrefix = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_BackPrefix->SetToolTip( _("Optional prefix for solder side reference designations (e.g. B_)") );

	gbSizer1->Add( m_BackPrefix, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_RemoveFrontPrefix = new wxCheckBox( this, wxID_ANY, _("Remove front prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RemoveFrontPrefix->SetToolTip( _("If checked will remove the front side prefix\nin the front prefix box if present") );

	gbSizer1->Add( m_RemoveFrontPrefix, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_RemoveBackPrefix = new wxCheckBox( this, wxID_ANY, _("Remove back prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RemoveBackPrefix->SetToolTip( _("If checked will remove the Back side prefix\nin the back prefix box if present") );

	gbSizer1->Add( m_RemoveBackPrefix, wxGBPosition( 2, 3 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableCol( 4 );

	bSizerOpts->Add( gbSizer1, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bmainSizer->Add( bSizerOpts, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );

	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,300 ) );

	bSizerMessages->Add( m_MessageWindow, 1, wxEXPAND|wxLEFT|wxRIGHT, 10 );


	bmainSizer->Add( bSizerMessages, 1, wxEXPAND|wxTOP, 15 );

	wxBoxSizer* m_buttonsSizer;
	m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_buttonsSizer->Add( m_sdbSizer, 1, wxALL|wxEXPAND, 5 );


	bmainSizer->Add( m_buttonsSizer, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bmainSizer );
	this->Layout();
	bmainSizer->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_BOARD_REANNOTATE_BASE::OnClose ) );
	m_FrontPrefix->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::FilterFrontPrefix ), NULL, this );
	m_BackPrefix->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::FilterBackPrefix ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::OnCloseClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::OnApplyClick ), NULL, this );
}

DIALOG_BOARD_REANNOTATE_BASE::~DIALOG_BOARD_REANNOTATE_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_BOARD_REANNOTATE_BASE::OnClose ) );
	m_FrontPrefix->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::FilterFrontPrefix ), NULL, this );
	m_BackPrefix->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::FilterBackPrefix ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::OnCloseClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_BOARD_REANNOTATE_BASE::OnApplyClick ), NULL, this );

}
