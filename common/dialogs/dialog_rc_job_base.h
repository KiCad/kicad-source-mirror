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
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RC_JOB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RC_JOB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText18;
		wxChoice* m_choiceFormat;
		wxStaticText* m_staticText10;
		wxCheckBox* m_cbViolationErrors;
		wxCheckBox* m_cbViolationWarnings;
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputPath;
		wxCheckBox* m_cbHaltOutput;
		wxCheckBox* m_cbAllTrackViolations;
		wxCheckBox* m_cbSchParity;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFormatChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_RC_JOB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_RC_JOB_BASE();

};

