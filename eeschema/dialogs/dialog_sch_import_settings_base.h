///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Aug 10 2021)
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
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_IMPORT_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_IMPORT_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MainSizer;
		wxTextCtrl* m_filePathCtrl;
		wxBitmapButton* m_browseButton;
		wxBoxSizer* m_buttonsSizer;
		wxButton* m_selectAllButton;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelectAll( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxCheckBox* m_FormattingOpt;
		wxCheckBox* m_FieldNameTemplatesOpt;
		wxCheckBox* m_PinMapOpt;
		wxCheckBox* m_SeveritiesOpt;
		wxCheckBox* m_NetClassesOpt;

		DIALOG_SCH_IMPORT_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SCH_IMPORT_SETTINGS_BASE();

};

