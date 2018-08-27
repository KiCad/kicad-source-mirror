///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "pcb_layer_box_selector.h"

#include "dialog_graphic_item_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );
	
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bUpperSizer;
	bUpperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	m_fgUpperLeftGridSizer = new wxFlexGridSizer( 8, 3, 0, 0 );
	m_fgUpperLeftGridSizer->AddGrowableCol( 1 );
	m_fgUpperLeftGridSizer->SetFlexibleDirection( wxBOTH );
	m_fgUpperLeftGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_startXLabel = new wxStaticText( this, wxID_ANY, _("Start point X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_startXLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_startXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_startXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_startXUnits = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startXUnits->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_startXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_startYLabel = new wxStaticText( this, wxID_ANY, _("Start point Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_startYLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_startYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_startYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_startYUnits = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_startYUnits->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_startYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_endXLabel = new wxStaticText( this, wxID_ANY, _("End point X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_endXLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_endXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_endXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_endXUnits = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endXUnits->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_endXUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_endYLabel = new wxStaticText( this, wxID_ANY, _("End point Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_endYLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_endYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_endYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxEXPAND|wxLEFT|wxTOP, 5 );
	
	m_endYUnits = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_endYUnits->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_endYUnits, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_BezierPointC1XLabel = new wxStaticText( this, wxID_ANY, _("Bezier point C1 X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1XLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC1XLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_BezierC1X_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_BezierC1X_Ctrl, 0, wxTOP|wxBOTTOM|wxLEFT|wxEXPAND, 5 );
	
	m_BezierPointC1XUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1XUnit->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC1XUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_BezierPointC1YLabel = new wxStaticText( this, wxID_ANY, _("Bezier point C1 Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1YLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC1YLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_BezierC1Y_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_BezierC1Y_Ctrl, 0, wxTOP|wxBOTTOM|wxLEFT|wxEXPAND, 5 );
	
	m_BezierPointC1YUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC1YUnit->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC1YUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_BezierPointC2XLabel = new wxStaticText( this, wxID_ANY, _("Bezier point C2 X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2XLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC2XLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_BezierC2X_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_BezierC2X_Ctrl, 0, wxTOP|wxBOTTOM|wxLEFT|wxEXPAND, 5 );
	
	m_BezierPointC2XUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2XUnit->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC2XUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	m_BezierPointC2YLabel = new wxStaticText( this, wxID_ANY, _("Bezier point C2 Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2YLabel->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC2YLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_BezierC2Y_Ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_fgUpperLeftGridSizer->Add( m_BezierC2Y_Ctrl, 0, wxTOP|wxBOTTOM|wxLEFT|wxEXPAND, 5 );
	
	m_BezierPointC2YUnit = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_BezierPointC2YUnit->Wrap( -1 );
	m_fgUpperLeftGridSizer->Add( m_BezierPointC2YUnit, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );
	
	
	bUpperSizer->Add( m_fgUpperLeftGridSizer, 1, wxEXPAND|wxRIGHT, 30 );
	
	wxBoxSizer* bUpperRightSizer;
	bUpperRightSizer = new wxBoxSizer( wxVERTICAL );
	
	wxFlexGridSizer* fgUpperRightGridSizer;
	fgUpperRightGridSizer = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgUpperRightGridSizer->AddGrowableCol( 1 );
	fgUpperRightGridSizer->SetFlexibleDirection( wxBOTH );
	fgUpperRightGridSizer->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_angleLabel = new wxStaticText( this, wxID_ANY, _("Arc angle:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_angleLabel->Wrap( -1 );
	fgUpperRightGridSizer->Add( m_angleLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxTOP, 5 );
	
	m_angleCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgUpperRightGridSizer->Add( m_angleCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );
	
	m_angleUnits = new wxStaticText( this, wxID_ANY, _("deg"), wxDefaultPosition, wxDefaultSize, 0 );
	m_angleUnits->Wrap( -1 );
	fgUpperRightGridSizer->Add( m_angleUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_thicknessLabel = new wxStaticText( this, wxID_ANY, _("Line thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessLabel->Wrap( -1 );
	fgUpperRightGridSizer->Add( m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_thicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgUpperRightGridSizer->Add( m_thicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxALL, 5 );
	
	m_thicknessUnits = new wxStaticText( this, wxID_ANY, _("Unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_thicknessUnits->Wrap( -1 );
	fgUpperRightGridSizer->Add( m_thicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	fgUpperRightGridSizer->Add( m_LayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );
	
	m_LayerSelectionCtrl = new PCB_LAYER_BOX_SELECTOR( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgUpperRightGridSizer->Add( m_LayerSelectionCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3 );
	
	
	fgUpperRightGridSizer->Add( 0, 0, 1, wxEXPAND, 5 );
	
	
	bUpperRightSizer->Add( fgUpperRightGridSizer, 0, wxEXPAND, 5 );
	
	
	bUpperSizer->Add( bUpperRightSizer, 0, wxEXPAND, 5 );
	
	
	bMainSizer->Add( bUpperSizer, 1, wxEXPAND|wxALL, 10 );
	
	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline1, 0, wxEXPAND|wxRIGHT|wxLEFT, 10 );
	
	m_StandardButtonsSizer = new wxStdDialogButtonSizer();
	m_StandardButtonsSizerOK = new wxButton( this, wxID_OK );
	m_StandardButtonsSizer->AddButton( m_StandardButtonsSizerOK );
	m_StandardButtonsSizerCancel = new wxButton( this, wxID_CANCEL );
	m_StandardButtonsSizer->AddButton( m_StandardButtonsSizerCancel );
	m_StandardButtonsSizer->Realize();
	
	bMainSizer->Add( m_StandardButtonsSizer, 0, wxALIGN_RIGHT|wxALL, 5 );
	
	
	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::OnClose ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::OnInitDlg ) );
}

DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::~DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::OnClose ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::OnInitDlg ) );
	
}
