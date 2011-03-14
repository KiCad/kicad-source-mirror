///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_hotkeys_editor_base__
#define __dialog_hotkeys_editor_base__

#include <wx/intl.h>

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class HOTKEYS_EDITOR_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class HOTKEYS_EDITOR_DIALOG_BASE : public wxDialog 
{
	private:
	
	protected:
		wxGrid* m_hotkeyGrid;
		wxButton* m_OKButton;
		wxButton*  m_cancelButton;
		wxButton* m_undoButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnKeyPressed( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnRightClickOnCell( wxGridEvent& event ) { event.Skip(); }
		virtual void OnClickOnCell( wxGridEvent& event ) { event.Skip(); }
		virtual void OnOKClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void CancelClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void UndoClicked( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		HOTKEYS_EDITOR_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Hotkeys Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 304,235 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~HOTKEYS_EDITOR_DIALOG_BASE();
	
};

#endif //__dialog_hotkeys_editor_base__
