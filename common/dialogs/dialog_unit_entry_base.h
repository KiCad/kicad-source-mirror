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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class WX_UNIT_ENTRY_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class WX_UNIT_ENTRY_DIALOG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerMain;
		wxStaticText* m_label;
		wxTextCtrl* m_textCtrl;
		wxStaticText* m_unit_label;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

	public:

		WX_UNIT_ENTRY_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Title"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~WX_UNIT_ENTRY_DIALOG_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class WX_PT_ENTRY_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class WX_PT_ENTRY_DIALOG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerMain;
		wxStaticText* m_labelX;
		wxTextCtrl* m_textCtrlX;
		wxStaticText* m_unitsX;
		wxStaticText* m_labelY;
		wxTextCtrl* m_textCtrlY;
		wxStaticText* m_unitsY;
		wxButton* m_ButtonReset;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void ResetValues( wxCommandEvent& event ) { event.Skip(); }


	public:

		WX_PT_ENTRY_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Move Point to Location"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~WX_PT_ENTRY_DIALOG_BASE();

};

