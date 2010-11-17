///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_lib_edit_text_base__
#define __dialog_lib_edit_text_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_TEXT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_TEXT_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_TextValue;
		wxStaticText* m_TextSizeText;
		wxTextCtrl* m_TextSize;
		wxCheckBox* m_Orient;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_CommonUnit;
		wxCheckBox* m_CommonConvert;
		wxRadioBox* m_TextShapeOpt;
		wxRadioBox* m_TextHJustificationOpt;
		wxRadioBox* m_TextVJustificationOpt;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
	
	public:
		
		DIALOG_LIB_EDIT_TEXT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Library Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_EDIT_TEXT_BASE();
	
};

#endif //__dialog_lib_edit_text_base__
