///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun 18 2020)
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
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
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
		wxBitmapButton* m_buttonColor;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onColorButtonClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void resetDefaults( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_JUNCTION_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Junction Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_JUNCTION_PROPS_BASE();

};

