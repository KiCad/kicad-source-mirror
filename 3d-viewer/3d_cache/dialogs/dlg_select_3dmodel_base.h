///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Nov  1 2020)
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
#include <wx/dirctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DLG_SELECT_3D_MODELE_BASE
///////////////////////////////////////////////////////////////////////////////
class DLG_SELECT_3D_MODELE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxSplitterWindow* m_splitterWin;
		wxPanel* m_panelLeft;
		wxGenericDirCtrl* m_FileTree;
		wxPanel* m_pane3Dviewer;
		wxBoxSizer* m_Sizer3Dviewer;
		wxStaticText* m_stDirChoice;
		wxChoice* m_dirChoices;
		wxButton* m_cfgPathsButt;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnFileActivated( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectionChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void SetRootDir( wxCommandEvent& event ) { event.Skip(); }
		virtual void Cfg3DPaths( wxCommandEvent& event ) { event.Skip(); }


	public:

		DLG_SELECT_3D_MODELE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select 3D Model"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DLG_SELECT_3D_MODELE_BASE();

		void m_splitterWinOnIdle( wxIdleEvent& )
		{
			m_splitterWin->SetSashPosition( 300 );
			m_splitterWin->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DLG_SELECT_3D_MODELE_BASE::m_splitterWinOnIdle ), NULL, this );
		}

};

