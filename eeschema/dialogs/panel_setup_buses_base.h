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
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_BUSES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_BUSES_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* bMargins;
		wxBoxSizer* bLeftCol;
		wxStaticText* m_busesLabel;
		WX_GRID* m_aliasesGrid;
		STD_BITMAP_BUTTON* m_addAlias;
		STD_BITMAP_BUTTON* m_deleteAlias;
		wxStaticText* m_source;
		wxSimplebook* m_membersBook;
		wxPanel* membersPanel;
		wxStaticText* m_membersLabel;
		WX_GRID* m_membersGrid;
		STD_BITMAP_BUTTON* m_addMember;
		STD_BITMAP_BUTTON* m_removeMember;
		wxPanel* emptyPanel;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddMember( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveMember( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_BUSES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_BUSES_BASE();

};

