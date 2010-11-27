///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_orient_footprints_base__
#define __dialog_orient_footprints_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ORIENT_FOOTPRINTS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ORIENT_FOOTPRINTS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_OrientationCtrl;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_FilterPattern;
		
		wxCheckBox* m_ApplyToLocked;
		wxButton* m_buttonOK;
		wxButton* m_buttonClose;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_ORIENT_FOOTPRINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprints Orientation"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 403,160 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_ORIENT_FOOTPRINTS_BASE();
	
};

#endif //__dialog_orient_footprints_base__
