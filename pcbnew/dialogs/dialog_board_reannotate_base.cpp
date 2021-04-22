///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "wx_html_report_panel.h"

#include "dialog_board_reannotate_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_BOARD_REANNOTATE_BASE::DIALOG_BOARD_REANNOTATE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );

	m_notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_StandardOptions = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerOpts;
	bSizerOpts = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizerFpOrder;
	sbSizerFpOrder = new wxStaticBoxSizer( new wxStaticBox( m_StandardOptions, wxID_ANY, _("Footprint Order") ), wxVERTICAL );

	wxFlexGridSizer* fgSizerButtons;
	fgSizerButtons = new wxFlexGridSizer( 2, 11, 0, 0 );
	fgSizerButtons->AddGrowableCol( 2 );
	fgSizerButtons->AddGrowableCol( 5 );
	fgSizerButtons->AddGrowableCol( 8 );
	fgSizerButtons->SetFlexibleDirection( wxBOTH );
	fgSizerButtons->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_Down_Right = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_Down_Right->SetValue( true );
	fgSizerButtons->Add( m_Down_Right, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_down_right_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_down_right_bitmap->SetToolTip( _("Horizontally: top left to bottom right") );

	fgSizerButtons->Add( reannotate_down_right_bitmap, 0, wxALL, 5 );


	fgSizerButtons->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Right_Down = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Right_Down, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_right_down_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_right_down_bitmap->SetToolTip( _("Horizontally: top right to bottom left") );

	fgSizerButtons->Add( reannotate_right_down_bitmap, 0, wxALL, 5 );


	fgSizerButtons->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Down_Left = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Down_Left, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_down_left_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_down_left_bitmap->SetToolTip( _("Horizontally: bottom left to top right") );

	fgSizerButtons->Add( reannotate_down_left_bitmap, 0, wxALL, 5 );


	fgSizerButtons->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Left_Down = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Left_Down, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_left_down_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_left_down_bitmap->SetToolTip( _("Horizontally:: bottom right to top left") );

	fgSizerButtons->Add( reannotate_left_down_bitmap, 0, wxALL, 5 );

	m_Up_Right = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Up_Right, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_up_right_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_up_right_bitmap->SetToolTip( _("Vertically: top left to bottom right") );

	fgSizerButtons->Add( reannotate_up_right_bitmap, 0, wxALL, 5 );


	fgSizerButtons->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Right_Up = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Right_Up, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_right_up_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_right_up_bitmap->SetToolTip( _("Vertically: top right to bottom left") );

	fgSizerButtons->Add( reannotate_right_up_bitmap, 0, wxALL, 5 );


	fgSizerButtons->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Up_Left = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Up_Left, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_up_left_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_up_left_bitmap->SetToolTip( _("Vertically: bottom left to top right") );

	fgSizerButtons->Add( reannotate_up_left_bitmap, 0, wxALL, 5 );


	fgSizerButtons->Add( 20, 0, 1, wxEXPAND, 5 );

	m_Left_Up = new wxRadioButton( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerButtons->Add( m_Left_Up, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	reannotate_left_up_bitmap = new wxStaticBitmap( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 48,48 ), 0 );
	reannotate_left_up_bitmap->SetToolTip( _("Vertically: bottom right to top left") );

	fgSizerButtons->Add( reannotate_left_up_bitmap, 0, wxALL, 5 );


	sbSizerFpOrder->Add( fgSizerButtons, 1, wxEXPAND, 5 );

	wxFlexGridSizer* fgSizerLocations;
	fgSizerLocations = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerLocations->AddGrowableCol( 1 );
	fgSizerLocations->SetFlexibleDirection( wxBOTH );
	fgSizerLocations->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText9 = new wxStaticText( sbSizerFpOrder->GetStaticBox(), wxID_ANY, _("Based on location of:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText9->Wrap( -1 );
	fgSizerLocations->Add( m_staticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxString m_locationChoiceChoices[] = { _("Footprint"), _("Reference Designator") };
	int m_locationChoiceNChoices = sizeof( m_locationChoiceChoices ) / sizeof( wxString );
	m_locationChoice = new wxChoice( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_locationChoiceNChoices, m_locationChoiceChoices, 0 );
	m_locationChoice->SetSelection( 0 );
	fgSizerLocations->Add( m_locationChoice, 0, wxALL|wxEXPAND, 5 );

	m_SortGridText = new wxStaticText( sbSizerFpOrder->GetStaticBox(), wxID_ANY, _("Round locations to:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SortGridText->Wrap( -1 );
	m_SortGridText->SetToolTip( _("Component position will be rounded\nto this grid before sorting.\nThis helps with misaligned parts.") );

	fgSizerLocations->Add( m_SortGridText, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );

	wxArrayString m_GridChoiceChoices;
	m_GridChoice = new wxChoice( sbSizerFpOrder->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, m_GridChoiceChoices, 0 );
	m_GridChoice->SetSelection( 0 );
	m_GridChoice->SetToolTip( _("Component position will be rounded\nto this grid before sorting.\nThis helps with misaligned parts.") );
	m_GridChoice->SetMinSize( wxSize( 150,-1 ) );

	fgSizerLocations->Add( m_GridChoice, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );


	sbSizerFpOrder->Add( fgSizerLocations, 0, wxEXPAND|wxTOP, 5 );


	bSizerOpts->Add( sbSizerFpOrder, 0, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizeScope;
	sbSizeScope = new wxStaticBoxSizer( new wxStaticBox( m_StandardOptions, wxID_ANY, _("Reannotation Scope") ), wxVERTICAL );

	wxFlexGridSizer* fgSizer6111;
	fgSizer6111 = new wxFlexGridSizer( 0, 5, 0, 0 );
	fgSizer6111->SetFlexibleDirection( wxVERTICAL );
	fgSizer6111->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_NONE );

	AnnotateLabel = new wxStaticText( sbSizeScope->GetStaticBox(), wxID_ANY, _("Reannotate:"), wxDefaultPosition, wxDefaultSize, 0 );
	AnnotateLabel->Wrap( -1 );
	fgSizer6111->Add( AnnotateLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_AnnotateAll = new wxRadioButton( sbSizeScope->GetStaticBox(), wxID_ANY, _("All"), wxDefaultPosition, wxDefaultSize, 0 );
	m_AnnotateAll->SetValue( true );
	fgSizer6111->Add( m_AnnotateAll, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_AnnotateFront = new wxRadioButton( sbSizeScope->GetStaticBox(), wxID_ANY, _("Front"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6111->Add( m_AnnotateFront, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_AnnotateBack = new wxRadioButton( sbSizeScope->GetStaticBox(), wxID_ANY, _("Back"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6111->Add( m_AnnotateBack, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_AnnotateSelection = new wxRadioButton( sbSizeScope->GetStaticBox(), wxID_ANY, _("Selection"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer6111->Add( m_AnnotateSelection, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	sbSizeScope->Add( fgSizer6111, 0, wxBOTTOM|wxTOP, 5 );


	bSizerOpts->Add( sbSizeScope, 0, wxALL|wxEXPAND, 5 );


	m_StandardOptions->SetSizer( bSizerOpts );
	m_StandardOptions->Layout();
	bSizerOpts->Fit( m_StandardOptions );
	m_notebook->AddPage( m_StandardOptions, _("Options"), true );
	m_Advanced = new wxPanel( m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerDesignatorOpts;
	bSizerDesignatorOpts = new wxBoxSizer( wxHORIZONTAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 0, 0 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( 20,-1 ) );

	m_FrontRefDesStartText = new wxStaticText( m_Advanced, wxID_ANY, _("Front reference start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontRefDesStartText->Wrap( -1 );
	m_FrontRefDesStartText->SetToolTip( _("Starting reference designation for front.") );

	gbSizer1->Add( m_FrontRefDesStartText, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_FrontRefDesStart = new wxTextCtrl( m_Advanced, wxID_ANY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontRefDesStart->SetToolTip( _("Default is 1") );
	m_FrontRefDesStart->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_FrontRefDesStart, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_BottomRefDesStartText = new wxStaticText( m_Advanced, wxID_ANY, _("Back reference start:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BottomRefDesStartText->Wrap( -1 );
	m_BottomRefDesStartText->SetToolTip( _("Blank continues from front or enter a number greater than the highest reference designation on the front.") );

	gbSizer1->Add( m_BottomRefDesStartText, wxGBPosition( 0, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_BackRefDesStart = new wxTextCtrl( m_Advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_BackRefDesStart->SetToolTip( _("Leave blank or zero, or enter a number greater than the highest reference designation on the front.") );
	m_BackRefDesStart->SetMinSize( wxSize( 100,-1 ) );

	gbSizer1->Add( m_BackRefDesStart, wxGBPosition( 0, 4 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

	m_RemoveFrontPrefix = new wxCheckBox( m_Advanced, wxID_ANY, _("Remove front prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RemoveFrontPrefix->SetToolTip( _("If checked will remove the front side prefix\nin the front prefix box if present") );

	gbSizer1->Add( m_RemoveFrontPrefix, wxGBPosition( 2, 0 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_RemoveBackPrefix = new wxCheckBox( m_Advanced, wxID_ANY, _("Remove back prefix"), wxDefaultPosition, wxDefaultSize, 0 );
	m_RemoveBackPrefix->SetToolTip( _("If checked will remove the Back side prefix\nin the back prefix box if present") );

	gbSizer1->Add( m_RemoveBackPrefix, wxGBPosition( 2, 3 ), wxGBSpan( 1, 2 ), wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

	m_FrontPrefixText = new wxStaticText( m_Advanced, wxID_ANY, _("Front prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontPrefixText->Wrap( -1 );
	m_FrontPrefixText->SetToolTip( _("Optional prefix for component side reference designations (e.g. F_)") );

	gbSizer1->Add( m_FrontPrefixText, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_FrontPrefix = new wxTextCtrl( m_Advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_FrontPrefix->SetToolTip( _("Optional prefix for component side reference designations (e.g. F_)") );

	gbSizer1->Add( m_FrontPrefix, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_BackPrefixText = new wxStaticText( m_Advanced, wxID_ANY, _("Back prefix:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BackPrefixText->Wrap( -1 );
	m_BackPrefixText->SetToolTip( _("Optional prefix for solder side reference designations (e.g. B_)") );

	gbSizer1->Add( m_BackPrefixText, wxGBPosition( 1, 3 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT, 5 );

	m_BackPrefix = new wxTextCtrl( m_Advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_BackPrefix->SetToolTip( _("Optional prefix for solder side reference designations (e.g. B_)") );

	gbSizer1->Add( m_BackPrefix, wxGBPosition( 1, 4 ), wxGBSpan( 1, 1 ), wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_ExcludeLocked = new wxCheckBox( m_Advanced, wxID_ANY, _("Exclude locked footprints"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExcludeLocked->SetToolTip( _("Locked footprints will not be reannotated") );

	gbSizer1->Add( m_ExcludeLocked, wxGBPosition( 4, 0 ), wxGBSpan( 1, 5 ), wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND, 5 );

	m_ExcludeListText = new wxStaticText( m_Advanced, wxID_ANY, _("Exclude references:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ExcludeListText->Wrap( -1 );
	m_ExcludeListText->SetToolTip( _("Do not re-annotate this type \nof reference (R means R*)") );

	gbSizer1->Add( m_ExcludeListText, wxGBPosition( 5, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_ExcludeList = new wxTextCtrl( m_Advanced, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_ExcludeList, wxGBPosition( 5, 1 ), wxGBSpan( 1, 4 ), wxEXPAND|wxLEFT|wxRIGHT, 5 );


	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableCol( 2 );
	gbSizer1->AddGrowableCol( 3 );
	gbSizer1->AddGrowableCol( 4 );

	bSizerDesignatorOpts->Add( gbSizer1, 1, wxALL|wxEXPAND, 5 );


	m_Advanced->SetSizer( bSizerDesignatorOpts );
	m_Advanced->Layout();
	bSizerDesignatorOpts->Fit( m_Advanced );
	m_notebook->AddPage( m_Advanced, _("Reference Designators"), false );

	bupperSizer->Add( m_notebook, 0, wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	wxBoxSizer* bSizerMessages;
	bSizerMessages = new wxBoxSizer( wxVERTICAL );

	m_MessageWindow = new WX_HTML_REPORT_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_MessageWindow->SetMinSize( wxSize( -1,150 ) );

	bSizerMessages->Add( m_MessageWindow, 1, wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bupperSizer->Add( bSizerMessages, 1, wxALL|wxEXPAND, 5 );


	bmainSizer->Add( bupperSizer, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 6 );

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
