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
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PCM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PCM_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_dialogNotebook;
		wxPanel* m_panelRepository;
		wxChoice* m_choiceRepository;
		wxButton* m_buttonManage;
		wxNotebook* m_contentNotebook;
		wxPanel* m_panelInstalledHolder;
		wxPanel* m_panelPending;
		WX_GRID* m_gridPendingActions;
		wxBitmapButton* m_discardActionButton;
		wxButton* m_refreshButton;
		wxButton* m_installLocalButton;
		wxButton* m_openPackageDirButton;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Apply;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnRepositoryChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnManageRepositoriesClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPendingActionsCellClicked( wxGridEvent& event ) { event.Skip(); }
		virtual void OnDiscardActionClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRefreshClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnInstallFromFileClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOpenPackageDirClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApplyChangesClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDiscardChangesClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCloseClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PCM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plugin And Content Manager"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PCM_BASE();

};

