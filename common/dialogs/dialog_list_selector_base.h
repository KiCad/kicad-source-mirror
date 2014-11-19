///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_LIST_SELECTOR_BASE_H__
#define __DIALOG_LIST_SELECTOR_BASE_H__

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
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class EDA_LIST_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class EDA_LIST_DIALOG_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_filterLabel;
		wxTextCtrl* m_filterBox;
		wxStaticText* m_staticText2;
		wxListCtrl* m_listBox;
		wxStaticText* m_staticTextMsg;
		wxTextCtrl* m_messages;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCloseEvent& event ) = 0;
		virtual void textChangeInFilterBox( wxCommandEvent& event ) = 0;
		virtual void onListItemActivated( wxListEvent& event ) = 0;
		virtual void onListItemSelected( wxListEvent& event ) = 0;
		virtual void onCancelClick( wxCommandEvent& event ) = 0;
		virtual void onOkClick( wxCommandEvent& event ) = 0;
		
	
	public:
		
		EDA_LIST_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 400,400 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~EDA_LIST_DIALOG_BASE();
	
};

#endif //__DIALOG_LIST_SELECTOR_BASE_H__
