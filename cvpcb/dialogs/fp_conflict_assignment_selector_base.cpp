///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 19 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "fp_conflict_assignment_selector_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );
	
	m_staticTextInfo = new wxStaticText( this, wxID_ANY, _("Footprint assignments from schematic netlist and symbol footprint association file (.cmp) are conflicting.\n\nPlease choose the assignment."), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticTextInfo->Wrap( -1 );
	bSizerMain->Add( m_staticTextInfo, 0, wxALL|wxEXPAND, 5 );
	
	m_listFp = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_HRULES|wxLC_REPORT|wxLC_SINGLE_SEL|wxLC_VRULES );
	bSizerMain->Add( m_listFp, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();
	
	bSizerMain->Add( m_sdbSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnSize ) );
	m_listFp->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnItemClicked ), NULL, this );
	m_listFp->Connect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnColumnClick ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnOKClick ), NULL, this );
}

DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::~DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnSize ) );
	m_listFp->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnItemClicked ), NULL, this );
	m_listFp->Disconnect( wxEVT_COMMAND_LIST_COL_CLICK, wxListEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnColumnClick ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnCancelClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR_BASE::OnOKClick ), NULL, this );
	
}
