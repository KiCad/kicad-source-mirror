///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  8 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_fp_lib_table_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_FP_LIB_TABLE_BASE::DIALOG_FP_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_splitter1 = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D|wxSP_3DBORDER );
	m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_FP_LIB_TABLE_BASE::m_splitter1OnIdle ), NULL, this );
	m_splitter1->SetMinimumPaneSize( 10 );
	
	m_top = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_top, wxID_ANY, _("Global and Project Scope") ), wxVERTICAL );
	
	m_auinotebook2 = new wxAuiNotebook( m_top, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE );
	m_panel4 = new wxPanel( m_auinotebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel4->SetToolTip( _("Module libraries which  are visible for all projects") );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_grid1 = new wxGrid( m_panel4, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid1->CreateGrid( 1, 4 );
	m_grid1->EnableEditing( true );
	m_grid1->EnableGridLines( true );
	m_grid1->EnableDragGridSize( false );
	m_grid1->SetMargins( 0, 0 );
	
	// Columns
	m_grid1->AutoSizeColumns();
	m_grid1->EnableDragColMove( false );
	m_grid1->EnableDragColSize( true );
	m_grid1->SetColLabelSize( 30 );
	m_grid1->SetColLabelValue( 0, _("Nickname") );
	m_grid1->SetColLabelValue( 1, _("Library Path") );
	m_grid1->SetColLabelValue( 2, _("Plugin") );
	m_grid1->SetColLabelValue( 3, _("Options") );
	m_grid1->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid1->AutoSizeRows();
	m_grid1->EnableDragRowSize( true );
	m_grid1->SetRowLabelSize( 80 );
	m_grid1->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid1->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer4->Add( m_grid1, 1, wxALL|wxEXPAND, 5 );
	
	
	m_panel4->SetSizer( bSizer4 );
	m_panel4->Layout();
	bSizer4->Fit( m_panel4 );
	m_auinotebook2->AddPage( m_panel4, _("Global Libraries"), false, wxNullBitmap );
	m_panel5 = new wxPanel( m_auinotebook2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxVERTICAL );
	
	m_grid11 = new wxGrid( m_panel5, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid11->CreateGrid( 1, 4 );
	m_grid11->EnableEditing( true );
	m_grid11->EnableGridLines( true );
	m_grid11->EnableDragGridSize( false );
	m_grid11->SetMargins( 0, 0 );
	
	// Columns
	m_grid11->AutoSizeColumns();
	m_grid11->EnableDragColMove( false );
	m_grid11->EnableDragColSize( true );
	m_grid11->SetColLabelSize( 30 );
	m_grid11->SetColLabelValue( 0, _("Nickname") );
	m_grid11->SetColLabelValue( 1, _("Library Path") );
	m_grid11->SetColLabelValue( 2, _("Plugin") );
	m_grid11->SetColLabelValue( 3, _("Options") );
	m_grid11->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid11->AutoSizeRows();
	m_grid11->EnableDragRowSize( true );
	m_grid11->SetRowLabelSize( 80 );
	m_grid11->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid11->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer5->Add( m_grid11, 0, wxALL, 5 );
	
	
	m_panel5->SetSizer( bSizer5 );
	m_panel5->Layout();
	bSizer5->Fit( m_panel5 );
	m_auinotebook2->AddPage( m_panel5, _("Project Specific Libraries"), true, wxNullBitmap );
	
	sbSizer3->Add( m_auinotebook2, 1, wxEXPAND | wxALL, 5 );
	
	wxBoxSizer* bSizer51;
	bSizer51 = new wxBoxSizer( wxHORIZONTAL );
	
	m_button1 = new wxButton( m_top, wxID_ANY, _("Append Row"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button1->SetToolTip( _("Add a pcb library row to this table") );
	
	bSizer51->Add( m_button1, 0, wxALL, 5 );
	
	m_button2 = new wxButton( m_top, wxID_ANY, _("Delete Row"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button2->SetToolTip( _("Remove a PCB library from this library table") );
	
	bSizer51->Add( m_button2, 0, wxALL, 5 );
	
	m_button3 = new wxButton( m_top, wxID_ANY, _("Move Up"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button3->SetToolTip( _("Move the currently selected row up one position") );
	
	bSizer51->Add( m_button3, 0, wxALL, 5 );
	
	m_button4 = new wxButton( m_top, wxID_ANY, _("Move Down"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button4->SetToolTip( _("Move the currently selected row down one position") );
	
	bSizer51->Add( m_button4, 0, wxALL, 5 );
	
	
	sbSizer3->Add( bSizer51, 0, wxALIGN_CENTER, 5 );
	
	
	m_top->SetSizer( sbSizer3 );
	m_top->Layout();
	sbSizer3->Fit( m_top );
	m_bottom = new wxPanel( m_splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_bottom, wxID_ANY, _("Path Substitutions") ), wxVERTICAL );
	
	m_grid2 = new wxGrid( m_bottom, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	
	// Grid
	m_grid2->CreateGrid( 2, 2 );
	m_grid2->EnableEditing( true );
	m_grid2->EnableGridLines( true );
	m_grid2->EnableDragGridSize( false );
	m_grid2->SetMargins( 0, 0 );
	
	// Columns
	m_grid2->SetColSize( 0, 150 );
	m_grid2->SetColSize( 1, 500 );
	m_grid2->EnableDragColMove( false );
	m_grid2->EnableDragColSize( true );
	m_grid2->SetColLabelSize( 30 );
	m_grid2->SetColLabelValue( 0, _("Category") );
	m_grid2->SetColLabelValue( 1, _("Path Segment") );
	m_grid2->SetColLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Rows
	m_grid2->EnableDragRowSize( true );
	m_grid2->SetRowLabelSize( 40 );
	m_grid2->SetRowLabelValue( 0, _("%S") );
	m_grid2->SetRowLabelValue( 1, _("%P") );
	m_grid2->SetRowLabelAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
	
	// Label Appearance
	
	// Cell Defaults
	m_grid2->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	sbSizer1->Add( m_grid2, 0, wxALL, 5 );
	
	
	bSizer8->Add( sbSizer1, 1, wxALL|wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( m_bottom, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( m_bottom, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	
	bSizer8->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );
	
	
	m_bottom->SetSizer( bSizer8 );
	m_bottom->Layout();
	bSizer8->Fit( m_bottom );
	m_splitter1->SplitHorizontally( m_top, m_bottom, 254 );
	bSizer1->Add( m_splitter1, 2, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );
	
	this->Centre( wxBOTH );
}

DIALOG_FP_LIB_TABLE_BASE::~DIALOG_FP_LIB_TABLE_BASE()
{
}
