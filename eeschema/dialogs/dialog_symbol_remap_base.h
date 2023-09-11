///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_HTML_REPORT_PANEL;

#include "widgets/html_window.h"
#include "dialog_shim.h"
#include <wx/html/htmlwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SYMBOL_REMAP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SYMBOL_REMAP_BASE : public DIALOG_SHIM
{
	private:

	protected:
		HTML_WINDOW* m_htmlCtrl;
		wxButton* m_buttonRemap;
		wxButton* m_buttonClose;
		WX_HTML_REPORT_PANEL* m_messagePanel;

		// Virtual event handlers, override them in your derived class
		virtual void OnRemapSymbols( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateUIRemapButton( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYMBOL_REMAP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Remap Symbols"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SYMBOL_REMAP_BASE();

};

