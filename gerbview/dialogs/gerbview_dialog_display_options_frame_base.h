///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __gerbview_dialog_display_options_frame_base__
#define __gerbview_dialog_display_options_frame_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DISPLAY_OPTIONS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_BoxUnits;
		wxRadioBox* m_CursorShape;
		wxRadioBox* m_OptDisplayLines;
		wxRadioBox* m_OptDisplayFlashedItems;
		wxRadioBox* m_OptDisplayPolygons;
		
		wxRadioBox* m_ShowPageLimits;
		
		wxCheckBox* m_OptDisplayDCodes;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKBUttonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Gerbview Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 446,299 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_DISPLAY_OPTIONS_BASE();
	
};

#endif //__gerbview_dialog_display_options_frame_base__
