///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "html_window.h"
#include <wx/html/htmlwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <widgets/number_badge.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
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
		wxStaticBoxSizer* m_box;
		wxFlexGridSizer* m_fgSizer;
		HTML_WINDOW* m_htmlView;
		wxStaticText* m_staticTextShow;
		wxCheckBox* m_checkBoxShowAll;
		wxCheckBox* m_checkBoxShowErrors;
		NUMBER_BADGE* m_errorsBadge;
		wxCheckBox* m_checkBoxShowWarnings;
		NUMBER_BADGE* m_warningsBadge;
		wxCheckBox* m_checkBoxShowActions;
		wxCheckBox* m_checkBoxShowInfos;
		wxButton* m_btnSaveReportToFile;

		// Virtual event handlers, overide them in your derived class
		virtual void onRightClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowErrors( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowWarnings( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowActions( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckBoxShowInfos( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBtnSaveToFile( wxCommandEvent& event ) { event.Skip(); }


	public:

		WX_HTML_REPORT_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~WX_HTML_REPORT_PANEL_BASE();

};

