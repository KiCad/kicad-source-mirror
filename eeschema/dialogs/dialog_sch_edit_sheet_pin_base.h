///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SCH_EDIT_SHEET_PIN_BASE_H__
#define __DIALOG_SCH_EDIT_SHEET_PIN_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
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
class DIALOG_SCH_EDIT_SHEET_PIN_BASE : public DIALOG_SHIM
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
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_SCH_EDIT_SHEET_PIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Sheet Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 350,189 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SCH_EDIT_SHEET_PIN_BASE();
	
};

#endif //__DIALOG_SCH_EDIT_SHEET_PIN_BASE_H__
