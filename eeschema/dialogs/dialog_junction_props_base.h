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
class COLOR_SWATCH;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_JUNCTION_PROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_JUNCTION_PROPS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextDiameter;
		wxTextCtrl* m_textCtrlDiameter;
		wxStaticText* m_staticTextDiameterUnits;
		wxStaticText* m_staticTextColor;
		wxPanel* m_panel1;
		COLOR_SWATCH* m_colorSwatch;
		wxStaticText* m_helpLabel1;
		wxStaticText* m_helpLabel2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void resetDefaults( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_JUNCTION_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Junction Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_JUNCTION_PROPS_BASE();

};

