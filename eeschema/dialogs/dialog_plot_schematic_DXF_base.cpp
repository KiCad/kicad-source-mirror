///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_plot_schematic_DXF_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLOT_SCHEMATIC_DXF_BASE::DIALOG_PLOT_SCHEMATIC_DXF_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	
	bupperSizer->Add( 10, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* sbSizerMiddle;
	sbSizerMiddle = new wxBoxSizer( wxVERTICAL );
	
	wxString m_PlotColorCtrlChoices[] = { _("B/W"), _("Color") };
	int m_PlotColorCtrlNChoices = sizeof( m_PlotColorCtrlChoices ) / sizeof( wxString );
	m_PlotColorCtrl = new wxRadioBox( this, wxID_ANY, _("Plot Mode:"), wxDefaultPosition, wxDefaultSize, m_PlotColorCtrlNChoices, m_PlotColorCtrlChoices, 1, wxRA_SPECIFY_COLS );
	m_PlotColorCtrl->SetSelection( 0 );
	sbSizerMiddle->Add( m_PlotColorCtrl, 0, wxALL|wxEXPAND, 5 );
	
	m_Plot_Sheet_Ref_Ctrl = new wxCheckBox( this, wxID_ANY, _("Print page references"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Plot_Sheet_Ref_Ctrl->SetValue(true); 
	sbSizerMiddle->Add( m_Plot_Sheet_Ref_Ctrl, 0, wxALL|wxEXPAND, 5 );
	
	bupperSizer->Add( sbSizerMiddle, 1, wxEXPAND, 5 );
	
	
	bupperSizer->Add( 10, 10, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonPlotPage = new wxButton( this, wxID_ANY, _("&Plot Page"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPlotPage, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPlotAll = new wxButton( this, wxID_ANY, _("Plot A&LL"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPlotAll, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonClose, 0, wxALL|wxEXPAND, 5 );
	
	bupperSizer->Add( bbuttonsSizer, 1, wxALIGN_CENTER_VERTICAL, 5 );
	
	bmainSizer->Add( bupperSizer, 0, wxEXPAND, 5 );
	
	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Messages :"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bmainSizer->Add( m_staticText1, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_MsgBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	m_MsgBox->SetMinSize( wxSize( -1,150 ) );
	
	bmainSizer->Add( m_MsgBox, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	this->SetSizer( bmainSizer );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_buttonPlotPage->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_DXF_BASE::OnPlotCurrent ), NULL, this );
	m_buttonPlotAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_DXF_BASE::OnPlotAll ), NULL, this );
	m_buttonClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_DXF_BASE::OnCancelClick ), NULL, this );
}

DIALOG_PLOT_SCHEMATIC_DXF_BASE::~DIALOG_PLOT_SCHEMATIC_DXF_BASE()
{
	// Disconnect Events
	m_buttonPlotPage->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_DXF_BASE::OnPlotCurrent ), NULL, this );
	m_buttonPlotAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_DXF_BASE::OnPlotAll ), NULL, this );
	m_buttonClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_DXF_BASE::OnCancelClick ), NULL, this );
	
}
