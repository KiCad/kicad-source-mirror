///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_annotate_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_ANNOTATE_BASE::DIALOG_ANNOTATE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
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
	
	bupperSizer->Add( bscopeOptSizer, 0, wxEXPAND|wxLEFT|wxALIGN_RIGHT, 25 );
	
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
	
	m_rbSortBy_X_Position = new wxRadioButton( this, ID_SORT_BY_X_POSITION, _("Sort Components by &X Position"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	bSizerXpos->Add( m_rbSortBy_X_Position, 0, wxALL, 3 );
	
	
	bSizerXpos->Add( 0, 0, 1, wxEXPAND, 5 );
	
	annotate_down_right_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerXpos->Add( annotate_down_right_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxRIGHT, 12 );
	
	b_orderOptSizer->Add( bSizerXpos, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerYpos;
	bSizerYpos = new wxBoxSizer( wxHORIZONTAL );
	
	m_rbSortBy_Y_Position = new wxRadioButton( this, ID_SORT_BY_Y_POSITION, _("Sort Components by &Y Position"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( m_rbSortBy_Y_Position, 0, wxALL, 3 );
	
	
	bSizerYpos->Add( 0, 0, 1, wxEXPAND, 5 );
	
	annotate_right_down_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerYpos->Add( annotate_right_down_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 12 );
	
	b_orderOptSizer->Add( bSizerYpos, 0, wxEXPAND|wxRIGHT, 5 );
	
	wxBoxSizer* bSizerValuepos;
	bSizerValuepos = new wxBoxSizer( wxHORIZONTAL );
	
	rbSortByValue = new wxRadioButton( this, ID_SORT_BY_VALUE, _("Sort Components by &Value"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerValuepos->Add( rbSortByValue, 0, wxALL, 3 );
	
	
	bSizerValuepos->Add( 0, 0, 1, wxEXPAND, 5 );
	
	annotate_by_value_bitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizerValuepos->Add( annotate_by_value_bitmap, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 12 );
	
	b_orderOptSizer->Add( bSizerValuepos, 0, wxEXPAND|wxRIGHT, 5 );
	
	bupperSizer->Add( b_orderOptSizer, 1, wxEXPAND|wxLEFT, 25 );
	
	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bupperSizer->Add( m_staticline5, 0, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bButtonsSizer;
	bButtonsSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_btnClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnClose, 0, wxALL|wxEXPAND, 5 );
	
	m_btnClear = new wxButton( this, ID_CLEAR_ANNOTATION_CMP, _("Clear Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
	bButtonsSizer->Add( m_btnClear, 0, wxALL|wxEXPAND, 5 );
	
	m_btnApply = new wxButton( this, wxID_APPLY, _("Annotation"), wxDefaultPosition, wxDefaultSize, 0 );
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
