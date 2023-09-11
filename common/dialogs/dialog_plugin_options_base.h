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
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "widgets/html_window.h"
#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/listbox.h>
#include <wx/html/htmlwin.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PLUGIN_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PLUGIN_OPTIONS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_GRID* m_grid;
		STD_BITMAP_BUTTON* m_append_button;
		STD_BITMAP_BUTTON* m_delete_button;
		wxListBox* m_listbox;
		wxButton* m_append_choice_button;
		HTML_WINDOW* m_html;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onGridCellChange( wxGridEvent& event ) = 0;
		virtual void onSize( wxSizeEvent& event ) = 0;
		virtual void onUpdateUI( wxUpdateUIEvent& event ) = 0;
		virtual void onAppendRow( wxCommandEvent& event ) = 0;
		virtual void onDeleteRow( wxCommandEvent& event ) = 0;
		virtual void onListBoxItemSelected( wxCommandEvent& event ) = 0;
		virtual void onListBoxItemDoubleClicked( wxCommandEvent& event ) = 0;
		virtual void onAppendOption( wxCommandEvent& event ) = 0;


	public:

		DIALOG_PLUGIN_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER );

		~DIALOG_PLUGIN_OPTIONS_BASE();

};

