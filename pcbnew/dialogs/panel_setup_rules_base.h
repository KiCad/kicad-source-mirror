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
class STD_BITMAP_BUTTON;
class WX_HTML_REPORT_BOX;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/stc/stc.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/html/htmlwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_RULES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_RULES_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_title;
		wxHyperlinkCtrl* m_syntaxHelp;
		wxBoxSizer* m_topMargin;
		wxStyledTextCtrl* m_textEditor;
		STD_BITMAP_BUTTON* m_compileButton;
		WX_HTML_REPORT_BOX* m_errorsReport;

		// Virtual event handlers, override them in your derived class
		virtual void OnSyntaxHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnContextMenu( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnCompile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnErrorLinkClicked( wxHtmlLinkEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_RULES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_RULES_BASE();

};

