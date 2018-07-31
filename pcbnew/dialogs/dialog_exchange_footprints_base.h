///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EXCHANGE_FOOTPRINTS_BASE_H__
#define __DIALOG_EXCHANGE_FOOTPRINTS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXCHANGE_FOOTPRINTS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXCHANGE_FOOTPRINTS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxBoxSizer* m_mainSizer;
		wxGridBagSizer* m_upperSizer;
		wxRadioButton* m_matchAll;
		wxRadioButton* m_matchCurrentRef;
		wxRadioButton* m_matchSpecifiedRef;
		wxTextCtrl* m_specifiedRef;
		wxRadioButton* m_matchCurrentValue;
		wxRadioButton* m_matchSpecifiedValue;
		wxTextCtrl* m_specifiedValue;
		wxRadioButton* m_matchSpecifiedID;
		wxTextCtrl* m_specifiedID;
		wxBitmapButton* m_specifiedIDBrowseButton;
		wxBoxSizer* m_middleSizer;
		wxTextCtrl* m_newID;
		wxBitmapButton* m_newIDBrowseButton;
		WX_HTML_REPORT_PANEL* m_MessageWindow;
		wxButton* m_exportButton;
		wxButton* m_applyButton;
		wxButton* m_closeButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void updateMatchModeRadioButtons( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnMatchAllClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchRefClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchValueClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchIDClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ViewAndSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void RebuildCmpList( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApplyClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_EXCHANGE_FOOTPRINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("%s"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_EXCHANGE_FOOTPRINTS_BASE();
	
};

#endif //__DIALOG_EXCHANGE_FOOTPRINTS_BASE_H__
