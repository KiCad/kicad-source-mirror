///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __WX_HTML_REPORT_PANEL_BASE_H__
#define __WX_HTML_REPORT_PANEL_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/html/htmlwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class WX_HTML_REPORT_PANEL_BASE
///////////////////////////////////////////////////////////////////////////////
class WX_HTML_REPORT_PANEL_BASE : public wxPanel 
{
	private:
	
	protected:
		wxHtmlWindow* m_htmlView;
		wxStaticText* m_staticText3;
		wxCheckBox* m_checkBoxShowAll;
		wxCheckBox* m_checkBoxShowWarnings;
		wxCheckBox* m_checkBoxShowErrors;
		wxCheckBox* m_checkBoxShowInfos;
		wxCheckBox* m_checkBoxShowActions;
		wxButton* m_btnSaveReportToFile;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onCheckBoxShowAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowWarnings( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowErrors( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowInfos( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowActions( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBtnSaveToFile( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		WX_HTML_REPORT_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL ); 
		~WX_HTML_REPORT_PANEL_BASE();
	
};

#endif //__WX_HTML_REPORT_PANEL_BASE_H__
