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
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/treectrl.h>
#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_KICAD_DIFF_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_KICAD_DIFF_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_labelReference;
		wxStaticText* m_pathReference;
		wxStaticText* m_labelComparison;
		wxStaticText* m_pathComparison;
		wxStaticLine* m_separator;
		wxSplitterWindow* m_splitter;
		wxPanel* m_panelTree;
		wxTreeCtrl* m_treeChanges;
		wxPanel* m_panelDetail;
		wxStaticText* m_labelSummary;
		wxListCtrl* m_listProperties;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnTreeSelectionChanged( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnOK( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		DIALOG_KICAD_DIFF_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Compare Files"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 900,650 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_KICAD_DIFF_BASE();

		void m_splitterOnIdle( wxIdleEvent& )
		{
			m_splitter->SetSashPosition( 280 );
			m_splitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_KICAD_DIFF_BASE::m_splitterOnIdle ), NULL, this );
		}

};

