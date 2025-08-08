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
class STD_BITMAP_BUTTON;
class WX_HTML_REPORT_PANEL;

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
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_NETLIST_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_NETLIST_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxTextCtrl* m_NetlistFilenameCtrl;
		STD_BITMAP_BUTTON* m_browseButton;
		wxRadioBox* m_matchByTimestamp;
		wxCheckBox* m_cbDeleteExtraFootprints;
		wxCheckBox* m_cbUpdateFootprints;
		wxCheckBox* m_cbTransferGroups;
		wxCheckBox* m_cbOverrideLocks;
		wxCheckBox* m_cbDeleteShortingTracks;
		WX_HTML_REPORT_PANEL* m_MessageWindow;
		wxBoxSizer* m_buttonsSizer;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFilenameKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onBrowseNetlistFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOptionChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdatePCB( wxCommandEvent& event ) { event.Skip(); }
		virtual void onImportNetlist( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_NETLIST_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Netlist"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORT_NETLIST_BASE();

};

