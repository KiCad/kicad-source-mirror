///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "tuner_slider_base.h"

///////////////////////////////////////////////////////////////////////////

TUNER_SLIDER_BASE::TUNER_SLIDER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxHORIZONTAL );

	m_name = new wxStaticText( this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_name->Wrap( -1 );
	bSizerUpper->Add( m_name, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_closeBtn = new wxButton( this, wxID_ANY, _(" X "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	bSizerUpper->Add( m_closeBtn, 0, wxALL, 5 );


	bSizerMain->Add( bSizerUpper, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerSlider;
	bSizerSlider = new wxBoxSizer( wxVERTICAL );

	m_slider = new wxSlider( this, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_AUTOTICKS|wxSL_INVERSE|wxSL_LEFT|wxSL_VERTICAL );
	bSizerSlider->Add( m_slider, 1, 0, 5 );


	bSizerMiddle->Add( bSizerSlider, 0, wxEXPAND, 5 );

	wxGridSizer* gSizerTxtCtr;
	gSizerTxtCtr = new wxGridSizer( 0, 1, 0, 0 );

	m_maxText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_maxText->SetMinSize( wxSize( 70,-1 ) );

	gSizerTxtCtr->Add( m_maxText, 0, wxALIGN_TOP|wxALL, 5 );

	m_valueText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_valueText->SetMinSize( wxSize( 70,-1 ) );

	gSizerTxtCtr->Add( m_valueText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_minText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_minText->SetMinSize( wxSize( 70,-1 ) );

	gSizerTxtCtr->Add( m_minText, 0, wxALIGN_BOTTOM|wxALL, 5 );


	bSizerMiddle->Add( gSizerTxtCtr, 1, wxEXPAND, 5 );


	bSizerMain->Add( bSizerMiddle, 1, wxEXPAND, 5 );

	m_saveBtn = new wxButton( this, wxID_ANY, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerMain->Add( m_saveBtn, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );


	this->SetSizer( bSizerMain );
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
	m_maxText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMaxKillFocus ), NULL, this );
	m_maxText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMaxTextEnter ), NULL, this );
	m_valueText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onValueKillFocus ), NULL, this );
	m_valueText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onValueTextEnter ), NULL, this );
	m_minText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMinKillFocus ), NULL, this );
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
	m_maxText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMaxKillFocus ), NULL, this );
	m_maxText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMaxTextEnter ), NULL, this );
	m_valueText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onValueKillFocus ), NULL, this );
	m_valueText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onValueTextEnter ), NULL, this );
	m_minText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMinKillFocus ), NULL, this );
	m_minText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMinTextEnter ), NULL, this );
	m_saveBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onSave ), NULL, this );

}
