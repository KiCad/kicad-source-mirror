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
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText3;
		COLOR_SWATCH* m_zeroSwatch;
		wxStaticText* m_staticText15;
		COLOR_SWATCH* m_negativeSwatch;
		wxStaticText* m_staticText2;
		COLOR_SWATCH* m_positiveSwatch;
		wxStaticText* m_staticText21;
		COLOR_SWATCH* m_unknownSwatch;
		wxCheckBox* m_logScale;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

	public:

		DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Differential Skew Display Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DIFF_PHASE_SKEW_PROPERTIES_BASE();

};

