///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SCH_SHEET_PROPS_BASE_H__
#define __DIALOG_SCH_SHEET_PROPS_BASE_H__

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
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_SHEET_PROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_SHEET_PROPS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_textFileName;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_textFileNameSize;
		wxStaticText* m_staticFileNameSizeUnits;
		wxStaticText* m_staticText4;
		wxTextCtrl* m_textSheetName;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_textSheetNameSize;
		wxStaticText* m_staticSheetNameSizeUnits;
		wxStaticLine* m_staticline2;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticTextTimeStamp;
		wxTextCtrl* m_textCtrlTimeStamp;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
	
	public:
		
		DIALOG_SCH_SHEET_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Schematic Sheet Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 519,187 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SCH_SHEET_PROPS_BASE();
	
};

#endif //__DIALOG_SCH_SHEET_PROPS_BASE_H__
