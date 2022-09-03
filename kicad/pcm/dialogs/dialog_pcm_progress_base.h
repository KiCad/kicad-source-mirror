///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/wx_html_report_box.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/html/htmlwin.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PCM_PROGRESS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PCM_PROGRESS_BASE : public wxDialog
{
	private:

	protected:
		wxPanel* m_panelDownload;
		wxStaticText* m_downloadText;
		wxGauge* m_downloadGauge;
		wxPanel* m_panel2;
		wxGauge* m_overallGauge;
		WX_HTML_REPORT_BOX* m_reporter;
		wxButton* m_buttonCancel;
		wxButton* m_buttonClose;

		// Virtual event handlers, override them in your derived class
		virtual void OnCancelClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCloseClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PCM_PROGRESS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Applying Package Changes"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,500 ), long style = wxCAPTION );

		~DIALOG_PCM_PROGRESS_BASE();

};

