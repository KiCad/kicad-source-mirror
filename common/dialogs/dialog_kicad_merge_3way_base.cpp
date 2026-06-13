///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_kicad_merge_3way_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_KICAD_MERGE_3WAY_BASE::DIALOG_KICAD_MERGE_3WAY_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 850,650 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_labelIntro = new wxStaticText( this, wxID_ANY, _("Resolve each conflict by picking which side to keep, then click Apply."), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelIntro->Wrap( -1 );
	bMainSizer->Add( m_labelIntro, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 10 );

	m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_LIVE_UPDATE );
	m_splitter->SetSashGravity( 0.35 );
	m_splitter->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::m_splitterOnIdle ), NULL, this );

	m_panelConflicts = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bConflictsSizer;
	bConflictsSizer = new wxBoxSizer( wxVERTICAL );

	m_labelConflicts = new wxStaticText( m_panelConflicts, wxID_ANY, _("Conflicts"), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelConflicts->Wrap( -1 );
	bConflictsSizer->Add( m_labelConflicts, 0, wxALL|wxEXPAND, 5 );

	m_listConflicts = new wxListBox( m_panelConflicts, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE );
	m_listConflicts->SetMinSize( wxSize( 220,300 ) );

	bConflictsSizer->Add( m_listConflicts, 1, wxALL|wxEXPAND, 5 );


	m_panelConflicts->SetSizer( bConflictsSizer );
	m_panelConflicts->Layout();
	bConflictsSizer->Fit( m_panelConflicts );
	m_panelResolution = new wxPanel( m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bResolutionSizer;
	bResolutionSizer = new wxBoxSizer( wxVERTICAL );

	m_labelDetail = new wxStaticText( m_panelResolution, wxID_ANY, _("Select a conflict on the left to see details."), wxDefaultPosition, wxDefaultSize, 0 );
	m_labelDetail->Wrap( -1 );
	bResolutionSizer->Add( m_labelDetail, 0, wxALL|wxEXPAND, 5 );

	m_textDetail = new wxTextCtrl( m_panelResolution, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxTE_DONTWRAP );
	m_textDetail->SetMinSize( wxSize( 400,180 ) );

	bResolutionSizer->Add( m_textDetail, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sbResolutionBox;
	sbResolutionBox = new wxStaticBoxSizer( new wxStaticBox( m_panelResolution, wxID_ANY, _("Pick a side") ), wxHORIZONTAL );

	m_radioOurs = new wxRadioButton( sbResolutionBox->GetStaticBox(), wxID_ANY, _("Ours"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
	m_radioOurs->SetValue( true );
	sbResolutionBox->Add( m_radioOurs, 0, wxALL, 5 );

	m_radioTheirs = new wxRadioButton( sbResolutionBox->GetStaticBox(), wxID_ANY, _("Theirs"), wxDefaultPosition, wxDefaultSize, 0 );
	sbResolutionBox->Add( m_radioTheirs, 0, wxALL, 5 );

	m_radioAncestor = new wxRadioButton( sbResolutionBox->GetStaticBox(), wxID_ANY, _("Ancestor"), wxDefaultPosition, wxDefaultSize, 0 );
	sbResolutionBox->Add( m_radioAncestor, 0, wxALL, 5 );


	bResolutionSizer->Add( sbResolutionBox, 0, wxALL|wxEXPAND, 5 );


	m_panelResolution->SetSizer( bResolutionSizer );
	m_panelResolution->Layout();
	bResolutionSizer->Fit( m_panelResolution );
	m_splitter->SplitVertically( m_panelConflicts, m_panelResolution, 300 );
	bMainSizer->Add( m_splitter, 1, wxEXPAND|wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerApply = new wxButton( this, wxID_APPLY );
	m_sdbSizer->AddButton( m_sdbSizerApply );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	bMainSizer->Add( m_sdbSizer, 0, wxEXPAND|wxALL, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnClose ) );
	m_listConflicts->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnConflictSelected ), NULL, this );
	m_radioOurs->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnResolutionChanged ), NULL, this );
	m_radioTheirs->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnResolutionChanged ), NULL, this );
	m_radioAncestor->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnResolutionChanged ), NULL, this );
	m_sdbSizerApply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnApply ), NULL, this );
	m_sdbSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnCancel ), NULL, this );
}

DIALOG_KICAD_MERGE_3WAY_BASE::~DIALOG_KICAD_MERGE_3WAY_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnClose ) );
	m_listConflicts->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnConflictSelected ), NULL, this );
	m_radioOurs->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnResolutionChanged ), NULL, this );
	m_radioTheirs->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnResolutionChanged ), NULL, this );
	m_radioAncestor->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnResolutionChanged ), NULL, this );
	m_sdbSizerApply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnApply ), NULL, this );
	m_sdbSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_KICAD_MERGE_3WAY_BASE::OnCancel ), NULL, this );

}
