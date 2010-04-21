///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_print_using_printer_base__
#define __dialog_print_using_printer_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PRINT_USING_PRINTER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PRINT_USING_PRINTER_BASE : public wxDialog 
{
	private:
	
	protected:
		wxCheckBox* m_checkReference;
		wxCheckBox* m_checkMonochrome;
		wxButton* m_buttonPageSetup;
		wxButton* m_buttonPreview;
		wxButton* m_buttonPrint;
		wxButton* m_buttonQuit;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnPageSetup( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnPrintPreview( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnPrintButtonClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_PRINT_USING_PRINTER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Print"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PRINT_USING_PRINTER_BASE();
	
};

#endif //__dialog_print_using_printer_base__
