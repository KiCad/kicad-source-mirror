///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_lib_new_component_base__
#define __dialog_lib_new_component_base__

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
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_NEW_COMPONENT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_NEW_COMPONENT_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText6;
		
		wxStaticText* m_staticText2;
		
		wxTextCtrl* m_textName;
		
		
		wxStaticText* m_staticText3;
		
		wxTextCtrl* m_textReference;
		
		
		wxStaticText* m_staticText4;
		
		wxSpinCtrl* m_spinPartCount;
		
		
		wxCheckBox* m_checkHasConversion;
		
		wxCheckBox* m_checkIsPowerSymbol;
		
		wxCheckBox* m_checkLockItems;
		
		wxStaticText* m_staticText7;
		
		wxStaticText* m_staticText41;
		
		wxSpinCtrl* m_spinPinTextPosition;
		wxStaticText* m_staticText5;
		
		wxCheckBox* m_checkShowPinNumber;
		
		wxCheckBox* m_checkShowPinName;
		
		wxCheckBox* m_checkShowPinNameInside;
		
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		DIALOG_LIB_NEW_COMPONENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Component Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_NEW_COMPONENT_BASE();
	
};

#endif //__dialog_lib_new_component_base__
