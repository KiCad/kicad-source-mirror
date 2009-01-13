///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_SVG_print_base__
#define __dialog_SVG_print_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/gdicmn.h>
#include <wx/stattext.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SVG_PRINT_base
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SVG_PRINT_base : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			wxID_PRINT_CURRENT = 1000,
			wxID_PRINT_BOARD,
		};
		
		wxStaticBoxSizer* m_CopperLayersBoxSizer;
		wxStaticBoxSizer* m_TechnicalBoxSizer;
		wxStaticText* m_TextPenWidth;
		wxTextCtrl* m_DialogPenWidth;
		wxRadioBox* m_ModeColorOption;
		wxCheckBox* m_Print_Frame_Ref_Ctrl;
		wxCheckBox* m_PrintBoardEdgesCtrl;
		wxButton* m_buttonPrintSelected;
		wxButton* m_buttonBoard;
		wxButton* m_buttonQuit;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_FileNameCtrl;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_MessagesBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ){ event.Skip(); }
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnSetColorModeSelected( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonPrintSelectedClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonPrintBoardClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_SVG_PRINT_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Create SVG file"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 507,375 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SVG_PRINT_base();
	
};

#endif //__dialog_SVG_print_base__
