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
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_UPDATE_PCB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_UPDATE_PCB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_cbRelinkFootprints;
		wxCheckBox* m_cbTransferGroups;
		wxCheckBox* m_cbUpdateFootprints;
		wxCheckBox* m_cbDeleteExtraFootprints;
		wxCheckBox* m_cbOverrideLocks;
		wxCheckBox* m_cbUpdateFields;
		wxCheckBox* m_cbRemoveExtraFields;
		WX_HTML_REPORT_PANEL* m_messagePanel;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnOptionChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_UPDATE_PCB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Update PCB from Schematic"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_UPDATE_PCB_BASE();

};

