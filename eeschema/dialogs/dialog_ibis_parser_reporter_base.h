///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IBIS_PARSER_REPORTER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IBIS_PARSER_REPORTER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void OnCloseClick( wxCommandEvent& event ) { event.Skip(); }


	public:
		WX_HTML_REPORT_PANEL* m_messagePanel;

		DIALOG_IBIS_PARSER_REPORTER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Ibis parser"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IBIS_PARSER_REPORTER_BASE();

};

