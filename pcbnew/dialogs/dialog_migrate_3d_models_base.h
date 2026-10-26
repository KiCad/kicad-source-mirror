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
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/grid.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MIGRATE_3D_MODELS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MIGRATE_3D_MODELS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_headerLabel;
		wxStaticText* m_subHeader;
		wxStaticText* m_subHeader2;
		wxSplitterWindow* m_mainSplitter;
		wxPanel* m_leftPanel;
		wxSplitterWindow* m_splitter3;
		wxPanel* m_paneMissingModels;
		wxStaticText* m_missingLabel;
		wxListCtrl* m_missingList;
		wxStaticText* m_helpText;
		wxPanel* m_paneReplacements;
		wxStaticText* m_candidatesLabel;
		wxListCtrl* m_candidatesList;
		STD_BITMAP_BUTTON* m_addReplacement;
		wxStaticText* searchDirectoriesLabel;
		WX_GRID* m_searchDirsGrid;
		wxPanel* m_previewPanel;
		wxStaticText* m_previewLabel;
		STD_BITMAP_BUTTON* m_addSearchDir;
		STD_BITMAP_BUTTON* m_deleteSearchDir;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnMissingSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnCandidateSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnOpenExternalFileClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddSearchDirectoryClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveSearchDirectoryClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnKeepClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReplaceClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_MIGRATE_3D_MODELS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Migrate 3D Models"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MIGRATE_3D_MODELS_BASE();

};

