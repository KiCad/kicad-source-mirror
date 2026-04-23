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
#include <wx/textctrl.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/listbox.h>
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/combobox.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CREATE_NET_CHAIN_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CREATE_NET_CHAIN_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_headerLabel;
		wxSplitterWindow* m_splitter;
		wxPanel* m_gridPanel;
		wxTextCtrl* m_filterInput;
		WX_GRID* m_chainsGrid;
		wxPanel* m_membersPanel;
		wxStaticText* m_membersLabel;
		wxListBox* m_membersListBox;
		wxStaticLine* m_separator;
		wxStaticText* m_manualLabel;
		wxStaticText* m_fromLabel;
		wxComboBox* m_fromComponent;
		wxStaticText* m_toLabel;
		wxComboBox* m_toComponent;
		STD_BITMAP_BUTTON* m_findPathButton;
		wxStaticText* m_nameLabel;
		wxTextCtrl* m_nameInput;
		STD_BITMAP_BUTTON* m_refreshButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFilterChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChainSelected( wxGridEvent& event ) { event.Skip(); }
		virtual void OnFindPathClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRefreshClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_CREATE_NET_CHAIN_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Create Net Chain"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_CREATE_NET_CHAIN_BASE();

		void m_splitterOnIdle( wxIdleEvent& )
		{
			m_splitter->SetSashPosition( -100 );
			m_splitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( DIALOG_CREATE_NET_CHAIN_BASE::m_splitterOnIdle ), NULL, this );
		}

};

