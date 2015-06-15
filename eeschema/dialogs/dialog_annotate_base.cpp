///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_annotate_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ANNOTATE_BASE::DIALOG_ANNOTATE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextScope = new wxStaticText( this, wxID_ANY, _("Scope"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextScope->Wrap( -1 );
	m_staticTextScope->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bupperSizer->Add( m_staticTextScope, 0, wxALL, 6 );
	
	wxBoxSizer* bscopeOptSizer;
	bscopeOptSizer = new wxBoxSizer( wxVERTICAL );
	
	m_rbEntireSchematic = new wxRadioButton( this, ID_ENTIRE_SCHEMATIC, _("Use the &entire schematic"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bscopeOptSizer->Add( m_rbEntireSchematic, 0, wxALL, 3 );
	
	m_rbCurrPage = new wxRadioButton( this, ID_CURRENT_PAGE, _("Use the current &page only"), wxDefaultPosition, wxDefaultSize, 0 );
	bscopeOptSizer->Add( m_rbCurrPage, 0, wxALL, 3 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bscopeOptSizer->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_rbKeepAnnotation = new wxRadioButton( this, ID_KEEP_ANNOTATION, _("&Keep existing annotation"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bscopeOptSizer->Add( m_rbKeepAnnotation, 0, wxALL, 3 );
	
	m_rbResetAnnotation = new wxRadioButton( this, ID_RESET_ANNOTATION, _("&Reset existing annotation"), wxDefaultPosition, wxDefaultSize, 0 );
	bscopeOptSizer->Add( m_rbResetAnnotation, 0, wxALL, 3 );
	
	m_rbResetButLock = new wxRadioButton( this, ID_RESET_BUT_LOCK, _("R&eset, but do not swap any annotated multi-unit parts"), wxDefaultPosition, wxDefaultSize, 0 );
	bscopeOptSizer->Add( m_rbResetButLock, 0, wxALL, 3 );
	
	
	bupperSizer->Add( bscopeOptSizer, 0, wxEXPAND|wxLEFT, 25 );
	
	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bupperSizer->Add( m_staticline2, 0, wxEXPAND | wxALL, 5 );
	
	m_staticTextOrder = new wxStaticText( this, wxID_ANY, _("Annotation Order"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextOrder->Wrap( -1 );
	m_staticTextOrder->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bupperSizer->Add( m_staticTextOrder, 0, wxALL, 6 );
	
	wxBoxSizer* b_orderOptSizer;
	b_orderOptSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerXpos;
	bSizerXpos = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbSortBy_X_Position = new wxRadioButton( this, ID_SORT_BY_X_POSITION, _("Sort components by &X position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerXpos->Add( m_rbSortBy_X_Position, 0, wxALL, 3 );
	
	
	bSizerXpos->Add( 0, 0, 1, wxEXPAND, 5 );
	
	annotate_down_right_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerXpos->Add( annotate_down_right_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 12 );
	
	
	b_orderOptSizer->Add( bSizerXpos, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerYpos;
	bSizerYpos = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbSortBy_Y_Position = new wxRadioButton( this, ID_SORT_BY_Y_POSITION, _("Sort components by &Y position"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( m_rbSortBy_Y_Position, 0, wxALL, 3 );
	
	
	bSizerYpos->Add( 0, 0, 1, wxEXPAND, 5 );
	
	annotate_right_down_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( annotate_right_down_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 12 );
	
	
	b_orderOptSizer->Add( bSizerYpos, 0, wxEXPAND|wxRIGHT, 5 );
	
	
	bupperSizer->Add( b_orderOptSizer, 0, wxEXPAND|wxLEFT, 25 );
	
	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bupperSizer->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerAnnotAlgo;
	bSizerAnnotAlgo = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextAnnotateAlgo = new wxStaticText( this, wxID_ANY, _("Annotation Choice"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextAnnotateAlgo->Wrap( -1 );
	m_staticTextAnnotateAlgo->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerAnnotAlgo->Add( m_staticTextAnnotateAlgo, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizer1AlgoChoice;
	bSizer1AlgoChoice = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerChoiceInc;
	bSizerChoiceInc = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbUseIncremental = new wxRadioButton( this, ID_SORT_BY_X_POSITION, _("Use first free number in schematic"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerChoiceInc->Add( m_rbUseIncremental, 0, wxALL, 3 );
	
	
	bSizerChoiceInc->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer1AlgoChoice->Add( bSizerChoiceInc, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerChoiceIncBySheet;
	bSizerChoiceIncBySheet = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbUseSheetNum = new wxRadioButton( this, wxID_ANY, _("Start to  sheet number*100 and use first free number"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChoiceIncBySheet->Add( m_rbUseSheetNum, 0, wxALL, 3 );
	
	
	bSizerChoiceIncBySheet->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer1AlgoChoice->Add( bSizerChoiceIncBySheet, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizerChoiceIncBySheetLarge;
	bSizerChoiceIncBySheetLarge = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbStartSheetNumLarge = new wxRadioButton( this, wxID_ANY, _("Start to  sheet number*1000 and use first free number"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChoiceIncBySheetLarge->Add( m_rbStartSheetNumLarge, 0, wxALL, 3 );
	
	
	bSizerChoiceIncBySheetLarge->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizer1AlgoChoice->Add( bSizerChoiceIncBySheetLarge, 1, wxEXPAND, 5 );
	
	
	bSizerAnnotAlgo->Add( bSizer1AlgoChoice, 1, wxEXPAND|wxLEFT, 25 );
	
	
	bupperSizer->Add( bSizerAnnotAlgo, 0, wxEXPAND|wxRIGHT, 5 );
	
	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bupperSizer->Add( m_staticline4, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizerDldOptions;
	bSizerDldOptions = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextDlgOpts = new wxStaticText( this, wxID_ANY, _("Dialog"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextDlgOpts->Wrap( -1 );
	m_staticTextDlgOpts->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );
	
	bSizerDldOptions->Add( m_staticTextDlgOpts, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	wxBoxSizer* bSizerDlgChoices;
	bSizerDlgChoices = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizerChoiceClose;
	bSizerChoiceClose = new wxBoxSizer( wxHORIZONTAL );
	
	m_cbAutoCloseDlg = new wxCheckBox( this, wxID_ANY, _("Automatically close this dialog"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChoiceClose->Add( m_cbAutoCloseDlg, 0, wxALL, 5 );
	
	
	bSizerChoiceClose->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerDlgChoices->Add( bSizerChoiceClose, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerChoiceSilentMode;
	bSizerChoiceSilentMode = new wxBoxSizer( wxHORIZONTAL );
	
	m_cbUseSilentMode = new wxCheckBox( this, wxID_ANY, _("Silent mode"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerChoiceSilentMode->Add( m_cbUseSilentMode, 0, wxALL, 5 );
	
	
	bSizerChoiceSilentMode->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bSizerDlgChoices->Add( bSizerChoiceSilentMode, 1, wxEXPAND, 5 );
	
	
	bSizerDldOptions->Add( bSizerDlgChoices, 1, wxEXPAND|wxLEFT, 25 );
	
	m_staticline41 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerDldOptions->Add( m_staticline41, 0, wxEXPAND | wxALL, 5 );
	
	
	bupperSizer->Add( bSizerDldOptions, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnClose, 0, wxALL|wxEXPAND, 5 );
	
	m_btnClear = new wxButton( this, ID_CLEAR_ANNOTATION_CMP, _("Clear Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnClear, 0, wxALL|wxEXPAND, 5 );
	
	m_btnApply = new wxButton( this, wxID_APPLY, _("Annotate"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnApply, 0, wxALL|wxEXPAND, 5 );
	
	
	bupperSizer->Add( bButtonsSizer, 0, wxALIGN_CENTER_HORIZONTAL, 6 );
	
	
	bmainSizer->Add( bupperSizer, 1, wxALL|wxEXPAND, 6 );
	
	
	this->SetSizer( bmainSizer );
	this->Layout();
	bmainSizer->Fit( this );
	
	// Connect Events
	m_btnClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnCancelClick ), NULL, this );
	m_btnClear->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnClearAnnotationCmpClick ), NULL, this );
	m_btnApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnApplyClick ), NULL, this );
}

DIALOG_ANNOTATE_BASE::~DIALOG_ANNOTATE_BASE()
{
	// Disconnect Events
	m_btnClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnCancelClick ), NULL, this );
	m_btnClear->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnClearAnnotationCmpClick ), NULL, this );
	m_btnApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_ANNOTATE_BASE::OnApplyClick ), NULL, this );
	
}
