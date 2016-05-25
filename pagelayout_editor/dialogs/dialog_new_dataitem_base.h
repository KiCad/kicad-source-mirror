///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_NEW_DATAITEM_BASE_H__
#define __DIALOG_NEW_DATAITEM_BASE_H__

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
#include <wx/choice.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NEW_DATAITEM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NEW_DATAITEM_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticTextPosX;
		wxTextCtrl* m_textCtrlPosX;
		wxStaticText* m_staticTextPosY;
		wxTextCtrl* m_textCtrlPosY;
		wxStaticText* m_staticTextOrgPos;
		wxChoice* m_choiceCornerPos;
		wxStaticLine* m_staticline2;
		wxBoxSizer* m_SizerEndPosition;
		wxStaticText* m_staticTextEndX;
		wxTextCtrl* m_textCtrlEndX;
		wxStaticText* m_staticTextEndY;
		wxTextCtrl* m_textCtrlEndY;
		wxStaticText* m_staticTextOrgPos1;
		wxChoice* m_choiceCornerEnd;
		wxStaticLine* m_staticlineEndXY;
		wxBoxSizer* m_SizerText;
		wxStaticText* m_staticTextTitle;
		wxTextCtrl* m_textCtrlText;
		wxStaticLine* m_staticline3;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_NEW_DATAITEM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("New Item"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 328,335 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_NEW_DATAITEM_BASE();
	
};

#endif //__DIALOG_NEW_DATAITEM_BASE_H__
