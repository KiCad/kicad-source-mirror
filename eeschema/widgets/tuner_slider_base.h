///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class STD_BITMAP_BUTTON;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class TUNER_SLIDER_BASE
///////////////////////////////////////////////////////////////////////////////
class TUNER_SLIDER_BASE : public wxPanel
{
	private:

	protected:
		wxPanel* m_panel1;
		wxStaticText* m_name;
		wxChoice* m_modeChoice;
		wxStaticText* m_stepsLabel;
		wxSpinCtrl* m_stepCount;
		wxStaticLine* m_staticline4;
		BITMAP_BUTTON* m_e24;
		BITMAP_BUTTON* m_separator;
		BITMAP_BUTTON* m_e48;
		BITMAP_BUTTON* m_e96;
		BITMAP_BUTTON* m_e192;
		wxSlider* m_slider;
		wxTextCtrl* m_maxText;
		wxTextCtrl* m_valueText;
		wxTextCtrl* m_minText;
		wxButton* m_saveBtn;
		STD_BITMAP_BUTTON* m_closeBtn;

		// Virtual event handlers, override them in your derived class
		virtual void onRunModeChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onStepsChanged( wxSpinEvent& event ) { event.Skip(); }
		virtual void onStepsTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onESeries( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSliderScroll( wxScrollEvent& event ) { event.Skip(); }
		virtual void onSliderChanged( wxScrollEvent& event ) { event.Skip(); }
		virtual void onMaxKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onMaxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onValueKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onValueTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMinKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onMinTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSave( wxCommandEvent& event ) { event.Skip(); }
		virtual void onClose( wxCommandEvent& event ) { event.Skip(); }


	public:

		TUNER_SLIDER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_NONE|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~TUNER_SLIDER_BASE();

};

