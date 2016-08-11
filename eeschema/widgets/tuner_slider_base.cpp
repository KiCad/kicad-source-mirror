///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 24 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "tuner_slider_base.h"

///////////////////////////////////////////////////////////////////////////

TUNER_SLIDER_BASE::TUNER_SLIDER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );
	
	m_name = new wxStaticText( this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_name->Wrap( -1 );
	bSizer2->Add( m_name, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_closeBtn = new wxButton( this, wxID_ANY, _(" X "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizer2->Add( m_closeBtn, 0, wxALL, 5 );
	
	
	bSizer1->Add( bSizer2, 0, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_slider = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_INVERSE|wxSL_LEFT|wxSL_VERTICAL );
	bSizer4->Add( m_slider, 1, 0, 5 );
	
	
	bSizer3->Add( bSizer4, 0, wxEXPAND, 5 );
	
	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 1, 0, 0 );
	
	m_maxText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_maxText->SetMinSize( wxSize( 70,-1 ) );
	
	gSizer1->Add( m_maxText, 0, wxALIGN_TOP|wxALL, 5 );
	
	m_valueText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_valueText->SetMinSize( wxSize( 70,-1 ) );
	
	gSizer1->Add( m_valueText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	
	m_minText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_minText->SetMinSize( wxSize( 70,-1 ) );
	
	gSizer1->Add( m_minText, 0, wxALIGN_BOTTOM|wxALL, 5 );
	
	
	bSizer3->Add( gSizer1, 1, wxEXPAND, 5 );
	
	
	bSizer1->Add( bSizer3, 1, wxEXPAND, 5 );
	
	m_saveBtn = new wxButton( this, wxID_ANY, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_saveBtn, 0, wxALL, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();
	
	// Connect Events
	m_closeBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onClose ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_maxText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMaxTextEnter ), NULL, this );
	m_valueText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onValueTextEnter ), NULL, this );
	m_minText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMinTextEnter ), NULL, this );
	m_saveBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onSave ), NULL, this );
}

TUNER_SLIDER_BASE::~TUNER_SLIDER_BASE()
{
	// Disconnect Events
	m_closeBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onClose ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_maxText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMaxTextEnter ), NULL, this );
	m_valueText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onValueTextEnter ), NULL, this );
	m_minText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMinTextEnter ), NULL, this );
	m_saveBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onSave ), NULL, this );
	
}
