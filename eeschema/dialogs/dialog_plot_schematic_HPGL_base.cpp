///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_plot_schematic_HPGL_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_PLOT_SCHEMATIC_HPGL_BASE::DIALOG_PLOT_SCHEMATIC_HPGL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bmainSizer;
	bmainSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bupperSizer;
	bupperSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bleftSizer;
	bleftSizer = new wxBoxSizer( wxVERTICAL );
	
	wxString m_SizeOptionChoices[] = { _("Schematic size"), _("Page size A4"), _("Page size A3"), _("Page size A2"), _("Page size A1"), _("Page size A0"), _("Page size A"), _("Page size B"), _("Page size C"), _("Page size D"), _("Page size E") };
	int m_SizeOptionNChoices = sizeof( m_SizeOptionChoices ) / sizeof( wxString );
	m_SizeOption = new wxRadioBox( this, wxID_ANY, _("Plot Page Size:"), wxDefaultPosition, wxDefaultSize, m_SizeOptionNChoices, m_SizeOptionChoices, 1, wxRA_SPECIFY_COLS );
	m_SizeOption->SetSelection( 1 );
	bleftSizer->Add( m_SizeOption, 0, wxALL|wxEXPAND, 5 );
	
	m_Plot_Sheet_Ref_Ctrl = new wxCheckBox( this, wxID_ANY, _("Print page references"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Plot_Sheet_Ref_Ctrl->SetValue(true); 
	bleftSizer->Add( m_Plot_Sheet_Ref_Ctrl, 0, wxALL|wxEXPAND, 5 );
	
	bupperSizer->Add( bleftSizer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* sbSizerMiddle;
	sbSizerMiddle = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Pen control:") ), wxVERTICAL );
	
	m_penWidthTitle = new wxStaticText( this, wxID_ANY, _("Pen Width"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penWidthTitle->Wrap( -1 );
	sbSizer1->Add( m_penWidthTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_penWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer1->Add( m_penWidthCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_penSpeedTitle = new wxStaticText( this, wxID_ANY, _("Pen Speed ( cm/s )"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penSpeedTitle->Wrap( -1 );
	sbSizer1->Add( m_penSpeedTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_penSpeedCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 50, 20 );
	sbSizer1->Add( m_penSpeedCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	m_penNumTitle = new wxStaticText( this, wxID_ANY, _("Pen Number"), wxDefaultPosition, wxDefaultSize, 0 );
	m_penNumTitle->Wrap( -1 );
	sbSizer1->Add( m_penNumTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_penNumCtrl = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 8, 1 );
	sbSizer1->Add( m_penNumCtrl, 0, wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	sbSizerMiddle->Add( sbSizer1, 0, wxEXPAND|wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Page offset:") ), wxVERTICAL );
	
	m_offsetXTitle = new wxStaticText( this, wxID_ANY, _("Plot Offset X"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetXTitle->Wrap( -1 );
	sbSizer2->Add( m_offsetXTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PlotOrgPosition_X = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_PlotOrgPosition_X, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );
	
	m_offsetYTitle = new wxStaticText( this, wxID_ANY, _("Plot Offset Y"), wxDefaultPosition, wxDefaultSize, 0 );
	m_offsetYTitle->Wrap( -1 );
	sbSizer2->Add( m_offsetYTitle, 0, wxTOP|wxRIGHT|wxLEFT, 5 );
	
	m_PlotOrgPosition_Y = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	sbSizer2->Add( m_PlotOrgPosition_Y, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );
	
	sbSizerMiddle->Add( sbSizer2, 1, wxEXPAND|wxALL, 5 );
	
	bupperSizer->Add( sbSizerMiddle, 1, wxEXPAND, 5 );
	
	
	bupperSizer->Add( 10, 10, 0, 0, 5 );
	
	wxBoxSizer* bbuttonsSizer;
	bbuttonsSizer = new wxBoxSizer( wxVERTICAL );
	
	m_buttonPlotPage = new wxButton( this, wxID_ANY, _("&Plot Page"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPlotPage, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonPlotAll = new wxButton( this, wxID_ANY, _("Plot A&LL"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonPlotAll, 0, wxALL|wxEXPAND, 5 );
	
	m_buttonClose = new wxButton( this, wxID_CANCEL, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonClose, 0, wxALL|wxEXPAND, 5 );
	
	
	bbuttonsSizer->Add( 10, 10, 1, wxEXPAND, 5 );
	
	m_buttonOffset = new wxButton( this, wxID_ANY, _("&Accept Offset"), wxDefaultPosition, wxDefaultSize, 0 );
	bbuttonsSizer->Add( m_buttonOffset, 0, wxALL, 5 );
	
	bupperSizer->Add( bbuttonsSizer, 0, wxALIGN_CENTER_VERTICAL, 5 );
	
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
	m_SizeOption->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnPageSelected ), NULL, this );
	m_buttonPlotPage->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnPlotCurrent ), NULL, this );
	m_buttonPlotAll->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnPlotAll ), NULL, this );
	m_buttonClose->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnCancelClick ), NULL, this );
	m_buttonOffset->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::AcceptPlotOffset ), NULL, this );
}

DIALOG_PLOT_SCHEMATIC_HPGL_BASE::~DIALOG_PLOT_SCHEMATIC_HPGL_BASE()
{
	// Disconnect Events
	m_SizeOption->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnPageSelected ), NULL, this );
	m_buttonPlotPage->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnPlotCurrent ), NULL, this );
	m_buttonPlotAll->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnPlotAll ), NULL, this );
	m_buttonClose->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::OnCancelClick ), NULL, this );
	m_buttonOffset->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_PLOT_SCHEMATIC_HPGL_BASE::AcceptPlotOffset ), NULL, this );
	
}
