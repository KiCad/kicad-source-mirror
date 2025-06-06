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
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_textCommand;
		wxStyledTextCtrl* m_textCtrlCommand;
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputPath;
		wxCheckBox* m_cbRecordOutput;
		wxCheckBox* m_cbIgnoreExitCode;
		WX_GRID* m_path_subs_grid;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnRecordOutputClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSizeGrid( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Execute Command Job Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE();

};

