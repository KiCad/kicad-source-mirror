///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EXCHANGE_MODULES_BASE_H__
#define __DIALOG_EXCHANGE_MODULES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_MATCH_FP_ALL 4200
#define wxID_MATCH_FP_REF 4201
#define wxID_MATCH_FP_VAL 4202
#define wxID_MATCH_FP_ID 4203

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXCHANGE_MODULE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXCHANGE_MODULE_BASE : public DIALOG_SHIM
{
	private:
		wxButton* m_applyButton;
		wxButton* m_closeButton;
	
	protected:
		wxBoxSizer* m_localizationSizer;
		wxStaticText* m_updateModeTitle;
		wxStaticText* m_exchangeModeTitle;
		wxStaticText* m_updateModeVerb;
		wxStaticText* m_exchangeModeVerb;
		wxBoxSizer* m_allSizer;
		wxRadioButton* m_matchAll;
		wxBoxSizer* m_currentRefSizer;
		wxRadioButton* m_matchCurrentRef;
		wxBoxSizer* m_specifiedRefSizer;
		wxRadioButton* m_matchSpecifiedRef;
		wxTextCtrl* m_specifiedRef;
		wxBoxSizer* m_currentValueSizer;
		wxRadioButton* m_matchCurrentValue;
		wxBoxSizer* m_specifiedValueSizer;
		wxRadioButton* m_matchSpecifiedValue;
		wxTextCtrl* m_specifiedValue;
		wxBoxSizer* m_specifiedIDSizer;
		wxRadioButton* m_matchSpecifiedID;
		wxTextCtrl* m_specifiedID;
		wxBitmapButton* m_specifiedIDBrowseButton;
		wxBoxSizer* m_middleSizer;
		wxTextCtrl* m_newID;
		wxBitmapButton* m_newIDBrowseButton;
		WX_HTML_REPORT_PANEL* m_MessageWindow;
		wxButton* m_exportButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnMatchAllClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchRefClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchValueClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchIDClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ViewAndSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void RebuildCmpList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EXCHANGE_MODULE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("%s"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXCHANGE_MODULE_BASE();
	
};

#endif //__DIALOG_EXCHANGE_MODULES_BASE_H__
