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
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/splitter.h>
#include <wx/checkbox.h>
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
		wxSplitterWindow* m_mainSplitter;
		wxPanel* m_leftPanel;
		wxStaticText* m_missingLabel;
		wxListCtrl* m_missingList;
		wxPanel* m_rightContainerPanel;
		wxSplitterWindow* m_innerSplitter;
		wxPanel* m_middlePanel;
		wxStaticText* m_candidatesLabel;
		wxListCtrl* m_candidatesList;
		wxButton* m_addDirButton;
		wxButton* m_openFileButton;
		wxPanel* m_previewPanel;
		wxStaticText* m_previewLabel;
		wxCheckBox* m_doNotShowAgain;
		wxButton* m_replaceButton;
		wxButton* m_keepButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnMissingSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnCandidateSelected( wxListEvent& event ) { event.Skip(); }
		virtual void OnAddSearchDirectoryClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOpenExternalFileClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReplaceClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnKeepClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_MIGRATE_3D_MODELS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Migrate 3D Models"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 900,560 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MIGRATE_3D_MODELS_BASE();

		void m_mainSplitterOnIdle( wxIdleEvent& )
		{
			m_mainSplitter->SetSashPosition( 300 );
			m_mainSplitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::m_mainSplitterOnIdle ), NULL, this );
		}

		void m_innerSplitterOnIdle( wxIdleEvent& )
		{
			m_innerSplitter->SetSashPosition( 300 );
			m_innerSplitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_MIGRATE_3D_MODELS_BASE::m_innerSplitterOnIdle ), NULL, this );
		}

};

