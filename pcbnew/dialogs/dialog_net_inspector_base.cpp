///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_net_inspector_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_NET_INSPECTOR_BASE::DIALOG_NET_INSPECTOR_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bTopSizer;
	bTopSizer = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 5, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticTextFilter = new wxStaticText( this, wxID_ANY, _("Net name filter:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFilter->Wrap( -1 );
	fgSizer1->Add( m_staticTextFilter, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_textCtrlFilter = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( m_textCtrlFilter, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	bSizer7->Add( 20, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	fgSizer1->Add( bSizer7, 1, wxEXPAND, 5 );

	m_cbShowZeroPad = new wxCheckBox( this, wxID_ANY, _("Show zero pad nets"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbShowZeroPad->SetValue(true);
	fgSizer1->Add( m_cbShowZeroPad, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_groupBy = new wxCheckBox( this, wxID_ANY, _("Group by:"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_groupBy, 0, wxTOP|wxBOTTOM|wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_groupByText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_groupByText, 1, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );

	m_groupByKind = new wxComboBox( this, wxID_ANY, _("Wildcard"), wxDefaultPosition, wxSize( -1,-1 ), 0, NULL, wxCB_DROPDOWN|wxCB_READONLY|wxCB_SIMPLE );
	m_groupByKind->Append( _("Wildcard") );
	m_groupByKind->Append( _("RegEx") );
	m_groupByKind->Append( _("Wildcard Substr") );
	m_groupByKind->Append( _("RegEx Substr") );
	m_groupByKind->SetSelection( 0 );
	fgSizer1->Add( m_groupByKind, 0, wxALIGN_CENTER|wxFIXED_MINSIZE|wxRIGHT|wxLEFT, 5 );


	bTopSizer->Add( fgSizer1, 1, wxEXPAND, 5 );


	bSizerMain->Add( bTopSizer, 0, wxEXPAND|wxALL, 5 );

	wxBoxSizer* bMidSizer;
	bMidSizer = new wxBoxSizer( wxHORIZONTAL );


	bSizerMain->Add( bMidSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_netsList = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES|wxDV_MULTIPLE|wxDV_VERT_RULES );
	m_netsList->SetMinSize( wxSize( 500,300 ) );

	bSizerMain->Add( m_netsList, 1, wxEXPAND|wxLEFT|wxRIGHT, 10 );

	wxBoxSizer* bSizerListButtons;
	bSizerListButtons = new wxBoxSizer( wxHORIZONTAL );

	m_addNet = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerListButtons->Add( m_addNet, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	m_renameNet = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerListButtons->Add( m_renameNet, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	bSizerListButtons->Add( 20, 0, 0, wxEXPAND, 5 );

	m_deleteNet = new wxBitmapButton( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerListButtons->Add( m_deleteNet, 0, wxALL, 5 );


	bSizerListButtons->Add( 0, 0, 1, wxEXPAND, 5 );

	m_ReportButt = new wxButton( this, wxID_ANY, _("Create Report..."), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerListButtons->Add( m_ReportButt, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );


	bSizerMain->Add( bSizerListButtons, 0, wxEXPAND|wxALL, 5 );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerMain->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bSizerBottom->Add( m_sdbSizer, 1, wxALL|wxEXPAND, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxEXPAND|wxLEFT, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_textCtrlFilter->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_cbShowZeroPad->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_groupBy->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_groupByText->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_groupByKind->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_netsList->Connect( wxEVT_COMMAND_DATAVIEW_COLUMN_SORTED, wxDataViewEventHandler( DIALOG_NET_INSPECTOR_BASE::onSortingChanged ), NULL, this );
	m_netsList->Connect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_NET_INSPECTOR_BASE::onSelChanged ), NULL, this );
	m_netsList->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_NET_INSPECTOR_BASE::onListSize ), NULL, this );
	m_addNet->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onAddNet ), NULL, this );
	m_renameNet->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onRenameNet ), NULL, this );
	m_deleteNet->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onDeleteNet ), NULL, this );
	m_ReportButt->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onReport ), NULL, this );
}

DIALOG_NET_INSPECTOR_BASE::~DIALOG_NET_INSPECTOR_BASE()
{
	// Disconnect Events
	m_textCtrlFilter->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_cbShowZeroPad->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_groupBy->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_groupByText->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_groupByKind->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onFilterChange ), NULL, this );
	m_netsList->Disconnect( wxEVT_COMMAND_DATAVIEW_COLUMN_SORTED, wxDataViewEventHandler( DIALOG_NET_INSPECTOR_BASE::onSortingChanged ), NULL, this );
	m_netsList->Disconnect( wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler( DIALOG_NET_INSPECTOR_BASE::onSelChanged ), NULL, this );
	m_netsList->Disconnect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_NET_INSPECTOR_BASE::onListSize ), NULL, this );
	m_addNet->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onAddNet ), NULL, this );
	m_renameNet->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onRenameNet ), NULL, this );
	m_deleteNet->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onDeleteNet ), NULL, this );
	m_ReportButt->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_NET_INSPECTOR_BASE::onReport ), NULL, this );

}
