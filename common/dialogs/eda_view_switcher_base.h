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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class EDA_VIEW_SWITCHER_BASE
///////////////////////////////////////////////////////////////////////////////
class EDA_VIEW_SWITCHER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_stTitle;
		wxListBox* m_listBox;

	public:

		EDA_VIEW_SWITCHER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("View Preset Switcher"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxSTAY_ON_TOP );

		~EDA_VIEW_SWITCHER_BASE();

};

