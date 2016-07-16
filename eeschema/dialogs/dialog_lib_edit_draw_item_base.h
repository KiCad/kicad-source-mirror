///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIB_EDIT_DRAW_ITEM_BASE_H__
#define __DIALOG_LIB_EDIT_DRAW_ITEM_BASE_H__

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
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_DRAW_ITEM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_DRAW_ITEM_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_M_STATICTEXTSHARING = 1000
		};
		
		wxStaticText* m_staticText1;
		wxStaticText* m_staticWidth;
		wxTextCtrl* m_textWidth;
		wxStaticText* m_staticWidthUnits;
		wxStaticText* m_staticTextSharing;
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
		
		DIALOG_LIB_EDIT_DRAW_ITEM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Drawing Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIB_EDIT_DRAW_ITEM_BASE();
	
};

#endif //__DIALOG_LIB_EDIT_DRAW_ITEM_BASE_H__
