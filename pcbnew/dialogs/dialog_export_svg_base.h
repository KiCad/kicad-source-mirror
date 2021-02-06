///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Dec 30 2020)
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
#include <wx/checklst.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_SVG_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_SVG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		wxBitmapButton* m_browseButton;
		wxStaticText* m_staticTextCopperLayers;
		wxCheckListBox* m_CopperLayersList;
		wxStaticText* m_staticTextTechLayers;
		wxCheckListBox* m_TechnicalLayersList;
		wxRadioBox* m_ModeColorOption;
		wxRadioBox* m_rbSvgPageSizeOpt;
		wxCheckBox* m_PrintBoardEdgesCtrl;
		wxCheckBox* m_printMirrorOpt;
		wxRadioBox* m_rbFileOpt;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPlot( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_SVG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export SVG File"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_EXPORT_SVG_BASE();

};

