///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-75-g9786507b-dirty)
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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FP_EDIT_PAD_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FP_EDIT_PAD_TABLE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextPinNumbers;
		wxStaticText* m_pin_numbers_summary;
		wxStaticText* m_staticTextPinCount;
		wxStaticText* m_pin_count;
		wxStaticText* m_staticTextDuplicatePins;
		wxStaticText* m_duplicate_pins;
		WX_GRID* m_grid;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) = 0;
		virtual void OnCellChanged( wxGridEvent& event ) = 0;
		virtual void OnSelectCell( wxGridEvent& event ) = 0;
		virtual void OnSize( wxSizeEvent& event ) = 0;
		virtual void OnCancel( wxCommandEvent& event ) = 0;


	public:

		DIALOG_FP_EDIT_PAD_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pad Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FP_EDIT_PAD_TABLE_BASE();

};

