///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  7 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_edit_label_base__
#define __dialog_edit_label_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogLabelEditor_Base
///////////////////////////////////////////////////////////////////////////////
class DialogLabelEditor_Base : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			wxID_VALUE = 1000,
			wxID_SIZE,
		};
		
		wxStaticText* m_staticText1;
		wxTextCtrl* m_TextLabel;
		wxRadioBox* m_TextOrient;
		wxRadioBox* m_TextStyle;
		wxRadioBox* m_TextShape;
		wxStaticText* m_SizeTitle;
		wxTextCtrl* m_TextSize;
		
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnButtonOKClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCANCEL_Click( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		
		DialogLabelEditor_Base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Editor"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,300 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DialogLabelEditor_Base();
	
};

#endif //__dialog_edit_label_base__
