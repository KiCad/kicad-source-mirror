///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_exchange_modules_base__
#define __dialog_exchange_modules_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SELECTION_CLICKED 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXCHANGE_MODULE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXCHANGE_MODULE_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText6;
		wxTextCtrl* m_OldModule;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_OldValue;
		wxStaticText* m_staticText8;
		wxTextCtrl* m_NewModule;
		wxRadioBox* m_Selection;
		wxButton* m_OKbutton;
		wxButton* m_Quitbutton;
		wxButton* m_Browsebutton;
		wxStaticText* m_staticTextMsg;
		wxTextCtrl* m_WinMessages;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSelectionClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		virtual void BrowseAndSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EXCHANGE_MODULE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Exchange Modules"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 416,469 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXCHANGE_MODULE_BASE();
	
};

#endif //__dialog_exchange_modules_base__
