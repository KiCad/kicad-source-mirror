///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EXCHANGE_MODULES_BASE_H__
#define __DIALOG_EXCHANGE_MODULES_BASE_H__

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
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SELECTION_CLICKED 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXCHANGE_MODULE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXCHANGE_MODULE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText6;
		wxTextCtrl* m_OldModule;
		wxStaticText* m_staticText7;
		wxTextCtrl* m_OldValue;
		wxStaticText* m_staticText8;
		wxTextCtrl* m_NewModule;
		wxRadioBox* m_Selection;
		wxButton* m_OKbutton;
		wxButton* m_Quitbutton;
		wxButton* m_buttonCmpList;
		wxButton* m_Browsebutton;
		wxButton* m_buttonFPViewer;
		wxStaticText* m_staticTextMsg;
		wxTextCtrl* m_WinMessages;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSelectionClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		virtual void RebuildCmpList( wxCommandEvent& event ) { event.Skip(); }
		virtual void BrowseAndSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void ViewAndSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EXCHANGE_MODULE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Exchange Footprints"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 421,517 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXCHANGE_MODULE_BASE();
	
};

#endif //__DIALOG_EXCHANGE_MODULES_BASE_H__
