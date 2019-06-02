///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIB_NEW_COMPONENT_BASE_H__
#define __DIALOG_LIB_NEW_COMPONENT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_NEW_COMPONENT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_NEW_COMPONENT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textName;
		wxStaticText* m_staticTextDes;
		wxTextCtrl* m_textReference;
		wxStaticText* m_staticTextUnits;
		wxSpinCtrl* m_spinPartCount;
		wxCheckBox* m_checkLockItems;
		wxCheckBox* m_checkHasConversion;
		wxCheckBox* m_checkIsPowerSymbol;
		wxStaticText* m_staticText12;
		wxSpinCtrl* m_spinPinTextPosition;
		wxCheckBox* m_checkShowPinNumber;
		wxCheckBox* m_checkShowPinName;
		wxCheckBox* m_checkShowPinNameInside;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_LIB_NEW_COMPONENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("New Symbol"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIB_NEW_COMPONENT_BASE();
	
};

#endif //__DIALOG_LIB_NEW_COMPONENT_BASE_H__
