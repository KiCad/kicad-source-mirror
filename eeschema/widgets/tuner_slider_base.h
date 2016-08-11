///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 24 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __TUNER_SLIDER_BASE_H__
#define __TUNER_SLIDER_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
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
		wxStaticText* m_name;
		wxButton* m_closeBtn;
		wxSlider* m_slider;
		wxTextCtrl* m_maxText;
		wxTextCtrl* m_valueText;
		wxTextCtrl* m_minText;
		wxButton* m_saveBtn;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSliderChanged( wxScrollEvent& event ) { event.Skip(); }
		virtual void onMaxTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onValueTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMinTextEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSave( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		TUNER_SLIDER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 97,283 ), long style = wxTAB_TRAVERSAL ); 
		~TUNER_SLIDER_BASE();
	
};

#endif //__TUNER_SLIDER_BASE_H__
