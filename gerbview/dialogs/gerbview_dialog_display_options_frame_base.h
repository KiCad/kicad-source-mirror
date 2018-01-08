///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jan  2 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __GERBVIEW_DIALOG_DISPLAY_OPTIONS_FRAME_BASE_H__
#define __GERBVIEW_DIALOG_DISPLAY_OPTIONS_FRAME_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/stepped_slider.h"
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DISPLAY_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxBoxSizer* m_UpperSizer;
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_BoxUnits;
		wxRadioBox* m_OptDisplayFlashedItems;
		wxRadioBox* m_OptDisplayLines;
		wxRadioBox* m_OptDisplayPolygons;
		wxCheckBox* m_OptDisplayDCodes;
		wxRadioBox* m_ShowPageLimits;
		wxCheckBox* m_OptZoomNoCenter;
		wxCheckBox* m_OptMousewheelPan;
		wxStaticText* m_staticText1;
		STEPPED_SLIDER* m_scaleSlider;
		wxStaticText* m_staticText2;
		wxCheckBox* m_scaleAuto;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnScaleSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnScaleAuto( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKBUttonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Gerbview Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_DISPLAY_OPTIONS_BASE();
	
};

#endif //__GERBVIEW_DIALOG_DISPLAY_OPTIONS_FRAME_BASE_H__
