///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
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
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_STEP_PROCESS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_STEP_PROCESS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxTextCtrl* m_textCtrlLog;
		wxGauge* m_activityGauge;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void OnButtonPlot( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_STEP_PROCESS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("3D Export"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_STEP_PROCESS_BASE();

};

