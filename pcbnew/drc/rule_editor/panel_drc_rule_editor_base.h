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
class WX_HTML_REPORT_BOX;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/hyperlink.h>
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
/// Class PANEL_DRC_RULE_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_DRC_RULE_EDITOR_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* bContentSizer;
		wxBoxSizer* bBasicDetailSizer;
		wxStaticText* m_nameLabel;
		wxTextCtrl* m_nameCtrl;
		wxStaticText* m_commentLabel;
		wxTextCtrl* m_commentCtrl;
		wxBoxSizer* m_constraintSizer;
		wxStaticText* m_constraintHeaderTitle;
		wxStaticLine* m_staticline3;
		wxBoxSizer* m_constraintContentSizer;
		wxStaticText* m_conditionHeaderTitle;
                wxHyperlinkCtrl* m_syntaxHelp;
                wxStaticLine* m_staticline8;
                wxBoxSizer* m_conditionControlsSizer;
                wxStyledTextCtrl* m_textConditionCtrl;
                wxBitmapButton* m_checkSyntaxBtnCtrl;
                WX_HTML_REPORT_BOX* m_syntaxErrorReport;
		wxStaticText* m_staticText711;
		wxStaticLine* m_staticline111;
		wxBoxSizer* m_LayersComboBoxSizer;

		// Virtual event handlers, override them in your derived class
		virtual void onSyntaxHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void onContextMenu( wxMouseEvent& event ) { event.Skip(); }
		virtual void onCheckSyntax( wxCommandEvent& event ) { event.Skip(); }
		virtual void onErrorLinkClicked( wxHtmlLinkEvent& event ) { event.Skip(); }


	public:

		PANEL_DRC_RULE_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_DRC_RULE_EDITOR_BASE();

};

