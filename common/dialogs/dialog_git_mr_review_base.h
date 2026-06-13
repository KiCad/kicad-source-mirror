///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statline.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GIT_MR_REVIEW_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GIT_MR_REVIEW_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_labelBase;
		wxComboBox* m_comboBase;
		wxStaticText* m_labelHead;
		wxComboBox* m_comboHead;
		wxButton* m_buttonCompare;
		wxStaticLine* m_separator;
		wxStaticText* m_labelChanged;
		wxListCtrl* m_listFiles;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnCompareClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFileActivated( wxListEvent& event ) { event.Skip(); }


	public:
		wxStdDialogButtonSizer* m_sdbSizer;

		DIALOG_GIT_MR_REVIEW_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Compare Branches"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,550 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GIT_MR_REVIEW_BASE();

};

