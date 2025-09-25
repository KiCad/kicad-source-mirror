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

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EMBEDDED_FILES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EMBEDDED_FILES_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_filesGridSizer;
		WX_GRID* m_files_grid;
		wxBoxSizer* m_buttonsSizer;
		STD_BITMAP_BUTTON* m_browse_button;
		STD_BITMAP_BUTTON* m_delete_button;
		wxCheckBox* m_cbEmbedFonts;
		wxButton* m_export;

		// Virtual event handlers, override them in your derived class
		virtual void onGridRightClick( wxGridEvent& event ) { event.Skip(); }
		virtual void onAddEmbeddedFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDeleteEmbeddedFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFontEmbedClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onExportFiles( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_EMBEDDED_FILES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_EMBEDDED_FILES_BASE();

};

