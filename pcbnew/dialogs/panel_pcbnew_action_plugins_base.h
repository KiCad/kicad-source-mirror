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
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCBNEW_ACTION_PLUGINS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCBNEW_ACTION_PLUGINS_BASE : public wxPanel
{
	private:

	protected:
		WX_GRID* m_grid;
		STD_BITMAP_BUTTON* m_moveUpButton;
		STD_BITMAP_BUTTON* m_moveDownButton;
		STD_BITMAP_BUTTON* m_openDirectoryButton;
		STD_BITMAP_BUTTON* m_reloadButton;
		STD_BITMAP_BUTTON* m_showErrorsButton;

		// Virtual event handlers, override them in your derived class
		virtual void OnGridCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnMoveUpButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDownButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOpenDirectoryButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReloadButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowErrorsButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PCBNEW_ACTION_PLUGINS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PCBNEW_ACTION_PLUGINS_BASE();

};

