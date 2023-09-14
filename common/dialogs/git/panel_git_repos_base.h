///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-254-gc2ef7767)
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

#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GIT_REPOS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_REPOS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_staticText12;
		wxCheckBox* m_cbDefault;
		wxStaticText* m_authorLabel;
		wxTextCtrl* m_author;
		wxStaticText* m_authorEmailLabel;
		wxTextCtrl* m_authorEmail;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticText20;
		WX_GRID* m_grid;
		STD_BITMAP_BUTTON* m_btnAddRepo;
		STD_BITMAP_BUTTON* m_btnEditRepo;
		STD_BITMAP_BUTTON* m_btnDelete;

		// Virtual event handlers, override them in your derived class
		virtual void onDefaultClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onGridDClick( wxGridEvent& event ) { event.Skip(); }
		virtual void onAddClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onEditClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeleteClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_GIT_REPOS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_REPOS_BASE();

};

