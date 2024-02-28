///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SYNC_SHEET_PINS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SYNC_SHEET_PINS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_sizerMain;
		wxNotebook* m_notebook;
		wxStaticText* m_labelTip;
		wxButton* m_btnClose;

		// Virtual event handlers, override them in your derived class
		virtual void OnCloseBtnClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SYNC_SHEET_PINS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Synchronize sheet pins and hierarchical labels"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SYNC_SHEET_PINS_BASE();

};

