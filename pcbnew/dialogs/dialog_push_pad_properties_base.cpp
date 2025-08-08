///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_push_pad_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PUSH_PAD_PROPERTIES_BASE::DIALOG_PUSH_PAD_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Options") ), wxVERTICAL );

	m_Pad_Shape_Filter_CB = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Do not modify pads having a different shape"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Pad_Shape_Filter_CB->SetValue(true);
	sbSizer1->Add( m_Pad_Shape_Filter_CB, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Pad_Layer_Filter_CB = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Do not modify pads having different layers"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Pad_Layer_Filter_CB->SetValue(true);
	sbSizer1->Add( m_Pad_Layer_Filter_CB, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Pad_Orient_Filter_CB = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Do not modify pads having a different orientation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Pad_Orient_Filter_CB->SetValue(true);
	sbSizer1->Add( m_Pad_Orient_Filter_CB, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_Pad_Type_Filter_CB = new wxCheckBox( sbSizer1->GetStaticBox(), wxID_ANY, _("Do not modify pads having a different type"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Pad_Type_Filter_CB->SetValue(true);
	sbSizer1->Add( m_Pad_Type_Filter_CB, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	bLeftSizer->Add( sbSizer1, 1, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( bLeftSizer, 1, wxALL|wxEXPAND, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Apply = new wxButton( this, wxID_APPLY );
	m_sdbSizer1->AddButton( m_sdbSizer1Apply );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bMainSizer->Add( m_sdbSizer1, 0, wxALL|wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_sdbSizer1Apply->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PUSH_PAD_PROPERTIES_BASE::PadPropertiesAccept ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PUSH_PAD_PROPERTIES_BASE::PadPropertiesAccept ), NULL, this );
}

DIALOG_PUSH_PAD_PROPERTIES_BASE::~DIALOG_PUSH_PAD_PROPERTIES_BASE()
{
	// Disconnect Events
	m_sdbSizer1Apply->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PUSH_PAD_PROPERTIES_BASE::PadPropertiesAccept ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PUSH_PAD_PROPERTIES_BASE::PadPropertiesAccept ), NULL, this );

}
