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
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE : public DIALOG_SHIM
{
	private:
		wxBoxSizer* m_GeneralBoxSizer;

	protected:
		wxNotebook* m_sourceNotebook;
		wxCheckBox* m_cbReplaceExisting;
		wxCheckBox* m_cbGroupItems;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Generate Multichannel Rule Areas"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 548,328 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE();

};

