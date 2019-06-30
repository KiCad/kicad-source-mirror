///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec  1 2018)
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
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GEN_FOOTPRINT_POSITION_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GEN_FOOTPRINT_POSITION_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MainSizer;
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		wxBitmapButton* m_browseButton;
		wxRadioBox* m_rbFormat;
		wxRadioBox* m_radioBoxUnits;
		wxRadioBox* m_radioBoxFilesCount;
		wxCheckBox* m_forceSMDOpt;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenerate( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GEN_FOOTPRINT_POSITION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Generate Footprint Position Files"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 506,476 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GEN_FOOTPRINT_POSITION_BASE();

};

