///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 30 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_ENUM_PADS_BASE_H__
#define __DIALOG_ENUM_PADS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ENUM_PADS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ENUM_PADS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_lblPadPrefix;
		wxTextCtrl* m_padPrefix;
		wxStaticText* m_lblPadStartNum;
		wxSpinCtrl* m_padStartNum;
		wxStaticText* m_lblInfo;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
	
	public:
		
		DIALOG_ENUM_PADS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pad enumeration settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 340,240 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~DIALOG_ENUM_PADS_BASE();
	
};

#endif //__DIALOG_ENUM_PADS_BASE_H__
