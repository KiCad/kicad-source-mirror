///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_lib_edit_pin_base__
#define __dialog_lib_edit_pin_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_PIN_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_PIN_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textName;
		
		wxStaticText* m_staticText2;
		wxTextCtrl* m_textNameTextSize;
		wxStaticText* m_staticNameTextSizeUnits;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_textNumber;
		
		wxStaticText* m_staticText9;
		wxTextCtrl* m_textNumberTextSize;
		wxStaticText* m_staticNumberTextSizeUnits;
		wxStaticText* m_staticText5;
		wxChoice* m_choiceOrientation;
		
		wxStaticText* m_staticText11;
		wxTextCtrl* m_textLength;
		wxStaticText* m_staticLengthUnits;
		wxStaticText* m_staticText6;
		wxChoice* m_choiceElectricalType;
		
		
		
		
		wxStaticText* m_staticText7;
		wxChoice* m_choiceStyle;
		
		
		
		wxCheckBox* m_checkApplyToAllParts;
		wxCheckBox* m_checkApplyToAllConversions;
		wxCheckBox* m_checkShow;
		
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
	
	public:
		DIALOG_LIB_EDIT_PIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_EDIT_PIN_BASE();
	
};

#endif //__dialog_lib_edit_pin_base__
