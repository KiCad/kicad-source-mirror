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
class WX_GRID;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_PIN_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_PIN_TABLE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_GRID* m_grid;
		wxStdDialogButtonSizer* m_Buttons;
		wxButton* m_ButtonsOK;
		wxButton* m_ButtonsCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) = 0;
		virtual void OnCellEdited( wxGridEvent& event ) = 0;
		virtual void OnSize( wxSizeEvent& event ) = 0;
		virtual void OnCancel( wxCommandEvent& event ) = 0;


	public:

		DIALOG_SCH_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pin Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SCH_PIN_TABLE_BASE();

};

