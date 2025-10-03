///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/bitmap_button.h"
#include "widgets/std_bitmap_button.h"

#include "tuner_slider_base.h"

///////////////////////////////////////////////////////////////////////////

TUNER_SLIDER_BASE::TUNER_SLIDER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizerMain;
	bSizerMain = new wxBoxSizer( wxVERTICAL );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN|wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerUpper;
	bSizerUpper = new wxBoxSizer( wxVERTICAL );

	m_name = new wxStaticText( m_panel1, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
	m_name->Wrap( -1 );
	m_name->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizerUpper->Add( m_name, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizer6->Add( bSizerUpper, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	wxBoxSizer* bSizerMode;
	bSizerMode = new wxBoxSizer( wxHORIZONTAL );

	wxString m_modeChoiceChoices[] = { _("Single Run"), _("Multi Run") };
	int m_modeChoiceNChoices = sizeof( m_modeChoiceChoices ) / sizeof( wxString );
	m_modeChoice = new wxChoice( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_modeChoiceNChoices, m_modeChoiceChoices, 0 );
	m_modeChoice->SetSelection( 0 );
	bSizerMode->Add( m_modeChoice, 1, wxEXPAND|wxRIGHT, 5 );

	m_stepsLabel = new wxStaticText( m_panel1, wxID_ANY, _("Steps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stepsLabel->Wrap( -1 );
	bSizerMode->Add( m_stepsLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_stepCount = new wxSpinCtrl( m_panel1, wxID_ANY, wxT("2"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS|wxTE_PROCESS_ENTER, 2, 100, 2 );
	bSizerMode->Add( m_stepCount, 0, wxALIGN_CENTER_VERTICAL, 5 );


	bSizer6->Add( bSizerMode, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );

	m_staticline4 = new wxStaticLine( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer6->Add( m_staticline4, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_e24 = new BITMAP_BUTTON( m_panel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_e24->SetToolTip( _("Limit to E24 series values.") );

	bSizer7->Add( m_e24, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 5 );

	m_separator = new BITMAP_BUTTON( m_panel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_separator->SetToolTip( _("Bold") );

	bSizer7->Add( m_separator, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_e48 = new BITMAP_BUTTON( m_panel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_e48->SetToolTip( _("Limit to E48 series values.") );

	bSizer7->Add( m_e48, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_e96 = new BITMAP_BUTTON( m_panel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_e96->SetToolTip( _("Limit to E96 series values") );

	bSizer7->Add( m_e96, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_e192 = new BITMAP_BUTTON( m_panel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBORDER_NONE );
	m_e192->SetToolTip( _("Limit to E192 series values") );

	bSizer7->Add( m_e192, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	bSizer6->Add( bSizer7, 0, wxEXPAND|wxTOP|wxBOTTOM, 2 );

	wxBoxSizer* bSizerMiddle;
	bSizerMiddle = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizerSlider;
	bSizerSlider = new wxBoxSizer( wxVERTICAL );

	m_slider = new wxSlider( m_panel1, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_INVERSE|wxSL_LEFT|wxSL_VERTICAL );
	m_slider->SetMinSize( wxSize( -1,200 ) );

	bSizerSlider->Add( m_slider, 1, wxTOP|wxRIGHT|wxLEFT, 5 );


	bSizerMiddle->Add( bSizerSlider, 0, wxEXPAND, 5 );

	wxGridSizer* gSizerTxtCtr;
	gSizerTxtCtr = new wxGridSizer( 0, 1, 0, 0 );

	m_maxText = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_maxText->SetMinSize( wxSize( 70,-1 ) );

	gSizerTxtCtr->Add( m_maxText, 0, wxALIGN_TOP|wxALL, 5 );

	m_valueText = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_valueText->SetMinSize( wxSize( 70,-1 ) );

	gSizerTxtCtr->Add( m_valueText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_minText = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_minText->SetMinSize( wxSize( 70,-1 ) );

	gSizerTxtCtr->Add( m_minText, 0, wxALIGN_BOTTOM|wxALL, 5 );


	bSizerMiddle->Add( gSizerTxtCtr, 1, wxEXPAND|wxBOTTOM, 5 );


	bSizer6->Add( bSizerMiddle, 1, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerBottom;
	bSizerBottom = new wxBoxSizer( wxHORIZONTAL );

	m_saveBtn = new wxButton( m_panel1, wxID_ANY, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerBottom->Add( m_saveBtn, 1, wxEXPAND|wxRIGHT|wxLEFT, 3 );

	m_closeBtn = new STD_BITMAP_BUTTON( m_panel1, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	bSizerBottom->Add( m_closeBtn, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 8 );


	bSizer6->Add( bSizerBottom, 0, wxEXPAND|wxALL, 3 );


	m_panel1->SetSizer( bSizer6 );
	m_panel1->Layout();
	bSizer6->Fit( m_panel1 );
	bSizerMain->Add( m_panel1, 1, wxEXPAND|wxRIGHT, 8 );


	this->SetSizer( bSizerMain );
	this->Layout();
	bSizerMain->Fit( this );

	// Connect Events
	m_modeChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( TUNER_SLIDER_BASE::onRunModeChanged ), NULL, this );
	m_stepCount->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TUNER_SLIDER_BASE::onStepsChanged ), NULL, this );
	m_stepCount->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onStepsTextEnter ), NULL, this );
	m_e24->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_e48->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_e96->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_e192->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Connect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_maxText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMaxKillFocus ), NULL, this );
	m_maxText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMaxTextEnter ), NULL, this );
	m_valueText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onValueKillFocus ), NULL, this );
	m_valueText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onValueTextEnter ), NULL, this );
	m_minText->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMinKillFocus ), NULL, this );
	m_minText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMinTextEnter ), NULL, this );
	m_saveBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onSave ), NULL, this );
	m_closeBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onClose ), NULL, this );
}

TUNER_SLIDER_BASE::~TUNER_SLIDER_BASE()
{
	// Disconnect Events
	m_modeChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( TUNER_SLIDER_BASE::onRunModeChanged ), NULL, this );
	m_stepCount->Disconnect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler( TUNER_SLIDER_BASE::onStepsChanged ), NULL, this );
	m_stepCount->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onStepsTextEnter ), NULL, this );
	m_e24->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_e48->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_e96->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_e192->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onESeries ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_CHANGED, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderScroll ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_BOTTOM, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_LINEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_LINEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_PAGEDOWN, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_PAGEUP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_THUMBRELEASE, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_slider->Disconnect( wxEVT_SCROLL_TOP, wxScrollEventHandler( TUNER_SLIDER_BASE::onSliderChanged ), NULL, this );
	m_maxText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMaxKillFocus ), NULL, this );
	m_maxText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMaxTextEnter ), NULL, this );
	m_valueText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onValueKillFocus ), NULL, this );
	m_valueText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onValueTextEnter ), NULL, this );
	m_minText->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( TUNER_SLIDER_BASE::onMinKillFocus ), NULL, this );
	m_minText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( TUNER_SLIDER_BASE::onMinTextEnter ), NULL, this );
	m_saveBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onSave ), NULL, this );
	m_closeBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TUNER_SLIDER_BASE::onClose ), NULL, this );

}
