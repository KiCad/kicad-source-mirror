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
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class EDA_LIST_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class EDA_LIST_DIALOG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_listLabel;
		wxListCtrl* m_listBox;
		wxTextCtrl* m_filterBox;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onSize( wxSizeEvent& event ) = 0;
		virtual void onListItemActivated( wxListEvent& event ) = 0;
		virtual void textChangeInFilterBox( wxCommandEvent& event ) = 0;


	public:
		wxBoxSizer* m_ExtrasSizer;
		wxBoxSizer* m_ButtonsSizer;

		EDA_LIST_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~EDA_LIST_DIALOG_BASE();

};

