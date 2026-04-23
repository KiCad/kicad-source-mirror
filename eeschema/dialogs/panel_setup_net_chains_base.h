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

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/listbox.h>
#include <wx/splitter.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/notebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_NET_CHAINS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_NET_CHAINS_BASE : public wxPanel
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_chainsTab;
		wxStaticText* m_chainsHeader;
		wxSplitterWindow* m_chainsSplitter;
		wxPanel* m_chainsGridPanel;
		WX_GRID* m_chainsGrid;
		wxPanel* m_membersPanel;
		wxStaticText* m_membersLabel;
		wxListBox* m_membersListBox;
		STD_BITMAP_BUTTON* m_deleteChainButton;
		wxPanel* m_classesTab;
		wxStaticText* m_classesHeader;
		WX_GRID* m_classesGrid;
		STD_BITMAP_BUTTON* m_addClassButton;
		STD_BITMAP_BUTTON* m_renameClassButton;
		STD_BITMAP_BUTTON* m_deleteClassButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnChainGridSelectionChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void OnDeleteChainClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClassAddClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClassRenameClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnClassDeleteClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_NET_CHAINS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_NET_CHAINS_BASE();

		void m_chainsSplitterOnIdle( wxIdleEvent& )
		{
			m_chainsSplitter->SetSashPosition( -120 );
			m_chainsSplitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_SETUP_NET_CHAINS_BASE::m_chainsSplitterOnIdle ), NULL, this );
		}

};

