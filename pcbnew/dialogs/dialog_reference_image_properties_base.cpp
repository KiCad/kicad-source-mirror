///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_reference_image_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_REFERENCE_IMAGE_PROPERTIES_BASE::DIALOG_REFERENCE_IMAGE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* m_GeneralBoxSizer;
	m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );

	m_Notebook = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_PanelGeneral = new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizerProperties;
	bSizerProperties = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 3, 5 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );
	gbSizer1->SetEmptyCellSize( wxSize( -1,5 ) );

	m_XPosLabel = new wxStaticText( m_PanelGeneral, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosLabel->Wrap( -1 );
	gbSizer1->Add( m_XPosLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_ModPositionX = new wxTextCtrl( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_ModPositionX, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_XPosUnit = new wxStaticText( m_PanelGeneral, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_XPosUnit->Wrap( -1 );
	gbSizer1->Add( m_XPosUnit, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_YPosLabel = new wxStaticText( m_PanelGeneral, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosLabel->Wrap( -1 );
	gbSizer1->Add( m_YPosLabel, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_ModPositionY = new wxTextCtrl( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_ModPositionY, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_YPosUnit = new wxStaticText( m_PanelGeneral, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_YPosUnit->Wrap( -1 );
	gbSizer1->Add( m_YPosUnit, wxGBPosition( 1, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_LayerLabel = new wxStaticText( m_PanelGeneral, wxID_ANY, _("Associated layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	gbSizer1->Add( m_LayerLabel, wxGBPosition( 2, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( m_PanelGeneral, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	gbSizer1->Add( m_LayerSelectionCtrl, wxGBPosition( 2, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_cbLocked = new wxCheckBox( m_PanelGeneral, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	gbSizer1->Add( m_cbLocked, wxGBPosition( 4, 0 ), wxGBSpan( 1, 3 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );
	gbSizer1->AddGrowableRow( 0 );

	bSizerProperties->Add( gbSizer1, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );


	m_PanelGeneral->SetSizer( bSizerProperties );
	m_PanelGeneral->Layout();
	bSizerProperties->Fit( m_PanelGeneral );
	m_Notebook->AddPage( m_PanelGeneral, _("General"), false );

	m_GeneralBoxSizer->Add( m_Notebook, 1, wxEXPAND|wxALL, 10 );

	m_sdbSizerStdButtons = new wxStdDialogButtonSizer();
	m_sdbSizerStdButtonsOK = new wxButton( this, wxID_OK );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsOK );
	m_sdbSizerStdButtonsCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizerStdButtons->AddButton( m_sdbSizerStdButtonsCancel );
	m_sdbSizerStdButtons->Realize();

	m_GeneralBoxSizer->Add( m_sdbSizerStdButtons, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( m_GeneralBoxSizer );
	this->Layout();
	m_GeneralBoxSizer->Fit( this );

	this->Centre( wxBOTH );
}

DIALOG_REFERENCE_IMAGE_PROPERTIES_BASE::~DIALOG_REFERENCE_IMAGE_PROPERTIES_BASE()
{
}
