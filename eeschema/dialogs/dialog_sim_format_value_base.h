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
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SIM_FORMAT_VALUE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SIM_FORMAT_VALUE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_precisionLabel;
		wxSpinCtrl* m_precisionCtrl;
		wxStaticText* m_rangeLabel;
		wxChoice* m_rangeCtrl;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

	public:

		DIALOG_SIM_FORMAT_VALUE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Format %s"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SIM_FORMAT_VALUE_BASE();

};

