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
#include "widgets/wx_panel.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SELECTION_FILTER_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SELECTION_FILTER_BASE : public WX_PANEL
{
	private:

	protected:
		wxCheckBox* m_cbAllItems;
		wxCheckBox* m_cbLockedItems;
		wxCheckBox* m_cbFootprints;
		wxCheckBox* m_cbText;
		wxCheckBox* m_cbTracks;
		wxCheckBox* m_cbVias;
		wxCheckBox* m_cbPads;
		wxCheckBox* m_cbGraphics;
		wxCheckBox* m_cbZones;
		wxCheckBox* m_cbKeepouts;
		wxCheckBox* m_cbDimensions;
		wxCheckBox* m_cbPoints;
		wxCheckBox* m_cbOtherItems;

		// Virtual event handlers, override them in your derived class
		virtual void OnFilterChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SELECTION_FILTER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SELECTION_FILTER_BASE();

};

