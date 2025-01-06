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
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GENERATORS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GENERATORS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_Notebook;
		wxPanel* m_panelPage1;
		wxDataViewCtrl* m_dataview1;
		wxBoxSizer* m_sizerButtons;
		wxButton* m_rebuildSelected;
		wxButton* m_rebuildThisType;
		wxButton* m_rebuildAll;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnActivateDlg( wxActivateEvent& event ) { event.Skip(); }
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnChangingNotebookPage( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnRebuildSelectedClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRebuildTypeClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRebuildAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GENERATORS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Generator Objects"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GENERATORS_BASE();

};

