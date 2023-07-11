///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINT_ASSOCIATIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINT_ASSOCIATIONS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_libraryAssociationLabel;
		WX_GRID* m_gridLibrary;
		wxStaticText* m_symbolAssociationLabel;
		WX_GRID* m_gridSymbol;
		wxStdDialogButtonSizer* m_sdbControlSizer;
		wxButton* m_sdbControlSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void windowSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void drillGridSize( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_FOOTPRINT_ASSOCIATIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Associations"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FOOTPRINT_ASSOCIATIONS_BASE();

};

