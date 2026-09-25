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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINT_USERS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINT_USERS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_cbShowFilterMatches;
		wxCheckBox* m_cbShowFootprintFieldMatches;
		wxCheckBox* m_cbShowPinMapMatches;
		wxStaticText* m_statusText;
		wxDataViewListCtrl* m_footprintUserList;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

	public:

		DIALOG_FOOTPRINT_USERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint users"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 750,450 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FOOTPRINT_USERS_BASE();

};

