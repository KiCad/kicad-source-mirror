///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_INCREMENT_ANNOTATIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_INCREMENT_ANNOTATIONS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_firstRefDesLabel;
		wxStaticText* m_incrementLabel;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:
		wxTextCtrl* m_FirstRefDes;
		wxSpinCtrl* m_Increment;
		wxRadioButton* m_CurrentSheet;
		wxRadioButton* m_AllSheets;

		DIALOG_INCREMENT_ANNOTATIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Increment Annotations From"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_INCREMENT_ANNOTATIONS_BASE();

};

