///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/std_bitmap_button.h"

#include "eda_reorderable_list_dialog_base.h"

///////////////////////////////////////////////////////////////////////////

EDA_REORDERABLE_LIST_DIALOG_BASE::EDA_REORDERABLE_LIST_DIALOG_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 320,220 ), wxDefaultSize );

	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bTop;
	bTop = new wxBoxSizer( wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 3, 3, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 2 );
	fgSizer1->AddGrowableRow( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_availableListLabel = new wxStaticText( this, wxID_ANY, _("Available:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_availableListLabel->Wrap( -1 );
	fgSizer1->Add( m_availableListLabel, 0, 0, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	m_enabledListLabel = new wxStaticText( this, wxID_ANY, _("Enabled:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_enabledListLabel->Wrap( -1 );
	fgSizer1->Add( m_enabledListLabel, 0, 0, 5 );

	m_availableListBox = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_SIMPLE|wxVSCROLL );
	m_availableListBox->SetMinSize( wxSize( 140,20 ) );

	fgSizer1->Add( m_availableListBox, 1, wxEXPAND, 5 );

	wxBoxSizer* bMiddleButtons;
	bMiddleButtons = new wxBoxSizer( wxVERTICAL );

	m_btnAdd = new wxButton( this, wxID_ANY, _(">"), wxDefaultPosition, wxSize( 24,24 ), 0 );
	bMiddleButtons->Add( m_btnAdd, 0, wxBOTTOM, 10 );

	m_btnRemove = new wxButton( this, wxID_ANY, _("<"), wxDefaultPosition, wxSize( 24,24 ), 0 );
	bMiddleButtons->Add( m_btnRemove, 0, wxBOTTOM|wxALIGN_CENTER_HORIZONTAL, 20 );


	fgSizer1->Add( bMiddleButtons, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 10 );

	m_enabledListBox = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL|wxBORDER_SIMPLE|wxVSCROLL );
	m_enabledListBox->SetMinSize( wxSize( 140,20 ) );

	fgSizer1->Add( m_enabledListBox, 3, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );


	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	m_btnUp = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnUp->SetToolTip( _("Move up") );

	bSizer4->Add( m_btnUp, 0, 0, 5 );

	m_btnDown = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_btnDown->SetToolTip( _("Move down") );

	bSizer4->Add( m_btnDown, 0, wxLEFT, 5 );


	fgSizer1->Add( bSizer4, 0, wxEXPAND, 5 );


	bTop->Add( fgSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	bSizerMain->Add( bTop, 1, wxEXPAND, 5 );

	m_ButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	m_ButtonsSizer->Add( m_sdbSizer, 1, wxALL, 5 );


	bSizerMain->Add( m_ButtonsSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	m_availableListBox->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onAvailableListItemSelected ), NULL, this );
	m_btnAdd->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onAddItem ), NULL, this );
	m_btnRemove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onRemoveItem ), NULL, this );
	m_enabledListBox->Connect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onEnabledListItemSelected ), NULL, this );
	m_btnUp->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onMoveUp ), NULL, this );
	m_btnDown->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onMoveDown ), NULL, this );
}

EDA_REORDERABLE_LIST_DIALOG_BASE::~EDA_REORDERABLE_LIST_DIALOG_BASE()
{
	// Disconnect Events
	m_availableListBox->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onAvailableListItemSelected ), NULL, this );
	m_btnAdd->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onAddItem ), NULL, this );
	m_btnRemove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onRemoveItem ), NULL, this );
	m_enabledListBox->Disconnect( wxEVT_COMMAND_LIST_ITEM_SELECTED, wxListEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onEnabledListItemSelected ), NULL, this );
	m_btnUp->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onMoveUp ), NULL, this );
	m_btnDown->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EDA_REORDERABLE_LIST_DIALOG_BASE::onMoveDown ), NULL, this );

}
