///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_imported_layers_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_IMPORTED_LAYERS_BASE::DIALOG_IMPORTED_LAYERS_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerTop;
	bSizerTop = new wxBoxSizer( wxHORIZONTAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Unmatched Layers") ), wxHORIZONTAL );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->AddGrowableRow( 1 );
	fgSizer1->SetFlexibleDirection( wxVERTICAL );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_lblImportedLayers = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("Imported Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblImportedLayers->Wrap( -1 );
	fgSizer1->Add( m_lblImportedLayers, 0, wxALL, 5 );

	m_lblKicadLayers = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, _("KiCad Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_lblKicadLayers->Wrap( -1 );
	fgSizer1->Add( m_lblKicadLayers, 0, wxALL, 5 );

	m_unmatched_layers_list = new wxListCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxLC_NO_HEADER|wxLC_REPORT );
	m_unmatched_layers_list->SetMinSize( wxSize( 120,-1 ) );

	fgSizer1->Add( m_unmatched_layers_list, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_kicad_layers_list = new wxListCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxLC_NO_HEADER|wxLC_REPORT|wxLC_SINGLE_SEL );
	m_kicad_layers_list->SetMinSize( wxSize( 120,-1 ) );

	fgSizer1->Add( m_kicad_layers_list, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	sbSizer1->Add( fgSizer1, 1, wxEXPAND, 5 );


	bSizerTop->Add( sbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	m_button_add = new wxButton( this, wxID_ANY, _(">"), wxDefaultPosition, wxSize( 36,100 ), 0 );
	m_button_add->SetToolTip( _("Add selected layers to matched layers list.") );

	bSizer6->Add( m_button_add, 0, wxALL, 5 );

	m_button_remove = new wxButton( this, wxID_ANY, _("<"), wxDefaultPosition, wxSize( 36,100 ), 0 );
	m_button_remove->SetToolTip( _("Remove selected layers from matched layers list.") );

	bSizer6->Add( m_button_remove, 0, wxALL, 5 );

	m_button_removeall = new wxButton( this, wxID_ANY, _("<<"), wxDefaultPosition, wxSize( 36,50 ), 0 );
	m_button_removeall->SetToolTip( _("Remove all matched layers.") );

	bSizer6->Add( m_button_removeall, 0, wxALL, 5 );


	bSizerTop->Add( bSizer6, 0, wxEXPAND|wxTOP, 10 );

	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Matched Layers") ), wxHORIZONTAL );

	m_matched_layers_list = new wxListCtrl( sbSizer3->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxSize( -1,-1 ), wxLC_REPORT );
	m_matched_layers_list->SetMinSize( wxSize( 120,-1 ) );

	sbSizer3->Add( m_matched_layers_list, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bSizerTop->Add( sbSizer3, 1, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 5 );


	bSizerMain->Add( bSizerTop, 1, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_button_automatch = new wxButton( this, wxID_ANY, _("Auto-Match Layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_button_automatch->SetToolTip( _("Automatically match any unmatched layers to their KiCad equivalent.") );

	bSizerBottom->Add( m_button_automatch, 0, wxALL, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizer->Realize();

	bSizerBottom->Add( m_sdbSizer, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerBottom, 0, wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnClose ) );
	this->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnUpdateUI ) );
	m_unmatched_layers_list->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnUnMatchedDoubleClick ), NULL, this );
	m_kicad_layers_list->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnUnMatchedDoubleClick ), NULL, this );
	m_button_add->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnAddClicked ), NULL, this );
	m_button_remove->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnRemoveClicked ), NULL, this );
	m_button_removeall->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnRemoveAllClicked ), NULL, this );
	m_matched_layers_list->Connect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnMatchedDoubleClick ), NULL, this );
	m_button_automatch->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnAutoMatchLayersClicked ), NULL, this );
}

DIALOG_IMPORTED_LAYERS_BASE::~DIALOG_IMPORTED_LAYERS_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnClose ) );
	this->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnUpdateUI ) );
	m_unmatched_layers_list->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnUnMatchedDoubleClick ), NULL, this );
	m_kicad_layers_list->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnUnMatchedDoubleClick ), NULL, this );
	m_button_add->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnAddClicked ), NULL, this );
	m_button_remove->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnRemoveClicked ), NULL, this );
	m_button_removeall->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnRemoveAllClicked ), NULL, this );
	m_matched_layers_list->Disconnect( wxEVT_COMMAND_LIST_ITEM_ACTIVATED, wxListEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnMatchedDoubleClick ), NULL, this );
	m_button_automatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_IMPORTED_LAYERS_BASE::OnAutoMatchLayersClicked ), NULL, this );

}
