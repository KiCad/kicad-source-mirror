///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_MODEDIT_OPTIONS_BASE_H__
#define __DIALOG_MODEDIT_OPTIONS_BASE_H__

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
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MODEDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MODEDIT_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText281;
		wxStaticText* m_EdgeModEWidthTitle;
		wxTextCtrl* m_OptModuleGrLineWidth;
		wxStaticText* m_staticTextGrLineUnit;
		wxStaticText* m_TextModWidthTitle;
		wxTextCtrl* m_OptModuleTextWidth;
		wxStaticText* m_staticTextTextWidthUnit;
		wxStaticText* m_TextModSizeVTitle;
		wxTextCtrl* m_OptModuleTextVSize;
		wxStaticText* m_staticTextTextVSizeUnit;
		wxStaticText* m_TextModSizeHTitle;
		wxTextCtrl* m_OptModuleTextHSize;
		wxStaticText* m_staticTextTextHSizeUnit;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText28;
		wxStaticText* m_staticTextInfo;
		wxStaticText* m_staticTextRef;
		wxTextCtrl* m_textCtrlRefText;
		wxStaticText* m_staticTextRefLayer;
		wxChoice* m_choiceLayerReference;
		wxStaticText* m_staticText32;
		wxChoice* m_choiceVisibleReference;
		wxStaticText* m_staticTextValue;
		wxTextCtrl* m_textCtrlValueText;
		wxStaticText* m_staticTextValLayer;
		wxChoice* m_choiceLayerValue;
		wxStaticText* m_staticTextValVisibility;
		wxChoice* m_choiceVisibleValue;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_MODEDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Editor Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 502,352 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_MODEDIT_OPTIONS_BASE();
	
};

#endif //__DIALOG_MODEDIT_OPTIONS_BASE_H__
