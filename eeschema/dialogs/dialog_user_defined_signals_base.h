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
class STD_BITMAP_BUTTON;
class WX_GRID;
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_USER_DEFINED_SIGNALS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_USER_DEFINED_SIGNALS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		WX_GRID* m_grid;
		wxBoxSizer* bButtonSize;
		STD_BITMAP_BUTTON* m_addButton;
		STD_BITMAP_BUTTON* m_deleteButton;
		wxHyperlinkCtrl* m_syntaxHelp;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onGridCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void onAddSignal( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeleteSignal( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFormattingHelp( wxHyperlinkEvent& event ) { event.Skip(); }


	public:

		DIALOG_USER_DEFINED_SIGNALS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("User-defined Signals"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_USER_DEFINED_SIGNALS_BASE();

};

