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
#include "dialog_shim.h"
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CONSTRAINTS_REPORTER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CONSTRAINTS_REPORTER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, overide them in your derived class
		virtual void OnOK( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_CONSTRAINTS_REPORTER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Constraints Resolution Report"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_CONSTRAINTS_REPORTER_BASE();

};

