///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIB_EDIT_TEXT_BASE_H__
#define __DIALOG_LIB_EDIT_TEXT_BASE_H__

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
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_TEXT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_TEXT_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_TextValue;
		wxButton* m_TextValueSelectButton;
		wxStaticText* m_TextSizeText;
		wxTextCtrl* m_TextSize;
		wxStaticText* m_PowerComponentValues;
		wxCheckBox* m_Orient;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_CommonUnit;
		wxCheckBox* m_CommonConvert;
		wxCheckBox* m_Invisible;
		wxRadioBox* m_TextShapeOpt;
		wxRadioBox* m_TextHJustificationOpt;
		wxRadioBox* m_TextVJustificationOpt;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseDialog( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnTextValueSelectButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_LIB_EDIT_TEXT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Library Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 615,343 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_LIB_EDIT_TEXT_BASE();
	
};

#endif //__DIALOG_LIB_EDIT_TEXT_BASE_H__
