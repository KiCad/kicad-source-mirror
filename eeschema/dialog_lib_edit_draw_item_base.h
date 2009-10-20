///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_lib_edit_draw_item_base__
#define __dialog_lib_edit_draw_item_base__

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
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_DRAW_ITEM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_DRAW_ITEM_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		
		wxStaticText* m_staticWidth;
		
		wxTextCtrl* m_textWidth;
		wxStaticText* m_staticWidthUnits;
		
		wxCheckBox* m_checkApplyToAllUnits;
		
		wxCheckBox* m_checkApplyToAllConversions;
		
		wxStaticText* m_staticText4;
		
		wxRadioButton* m_radioFillNone;
		wxRadioButton* m_radioFillForeground;
		wxRadioButton* m_radioFillBackground;
		
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
	
	public:
		DIALOG_LIB_EDIT_DRAW_ITEM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Drawing Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
		~DIALOG_LIB_EDIT_DRAW_ITEM_BASE();
	
};

#endif //__dialog_lib_edit_draw_item_base__
