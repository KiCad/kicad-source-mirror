///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/html_window.h"
#include <wx/html/htmlwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <widgets/number_badge.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

#include "kicommon.h"

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class WX_HTML_REPORT_PANEL_BASE
///////////////////////////////////////////////////////////////////////////////
class KICOMMON_API WX_HTML_REPORT_PANEL_BASE : public wxPanel
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

		// Virtual event handlers, override them in your derived class
		virtual void onRightClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBtnSaveToFile( wxCommandEvent& event ) { event.Skip(); }


	public:

		WX_HTML_REPORT_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~WX_HTML_REPORT_PANEL_BASE();

};

