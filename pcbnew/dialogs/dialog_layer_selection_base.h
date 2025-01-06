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
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/statbox.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LAYER_SELECTION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LAYER_SELECTION_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxGrid* m_leftGridLayers;
		wxGrid* m_rightGridLayers;

		// Virtual event handlers, override them in your derived class
		virtual void OnMouseMove( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLeftGridCellClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnLeftButtonReleased( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnRightGridCellClick( wxGridEvent& event ) { event.Skip(); }


	public:

		DIALOG_LAYER_SELECTION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select Layer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_LAYER_SELECTION_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextTopLayer;
		wxGrid* m_leftGridLayers;
		wxStaticText* m_staticTextBottomLayer;
		wxGrid* m_rightGridLayers;
		wxBitmapButton* m_addToPresetsButton;
		WX_GRID* m_presetsGrid;
		STD_BITMAP_BUTTON* m_deleteRowButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Select Copper Layer Pair"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_COPPER_LAYER_PAIR_SELECTION_BASE();

};

