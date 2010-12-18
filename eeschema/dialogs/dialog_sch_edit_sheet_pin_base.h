///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_sch_edit_sheet_pin_base__
#define __dialog_sch_edit_sheet_pin_base__

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
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_EDIT_SHEET_PIN_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_EDIT_SHEET_PIN_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textName;
		
		wxStaticText* m_staticText2;
		wxTextCtrl* m_textHeight;
		wxStaticText* m_staticHeightUnits;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_textWidth;
		wxStaticText* m_staticWidthUnits;
		wxStaticText* m_staticText3;
		wxChoice* m_choiceConnectionType;
		
		
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
	
	public:
		
		DIALOG_SCH_EDIT_SHEET_PIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Sheet Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SCH_EDIT_SHEET_PIN_BASE();
	
};

#endif //__dialog_sch_edit_sheet_pin_base__
