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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_AUTOSAVE_RECOVERY_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_AUTOSAVE_RECOVERY_BASE : public wxDialog
{
	private:

	protected:
		wxBoxSizer* bMainSizer;
		wxStaticText* m_explanation;
		wxListCtrl* m_fileList;
		wxButton* m_btnRestore;
		wxButton* m_btnKeepCurrent;
		wxButton* m_btnKeepBoth;
		wxButton* m_btnCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnRestore( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnKeepCurrent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnKeepBoth( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_AUTOSAVE_RECOVERY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Auto-Save Recovery"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,400 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_AUTOSAVE_RECOVERY_BASE();

};

