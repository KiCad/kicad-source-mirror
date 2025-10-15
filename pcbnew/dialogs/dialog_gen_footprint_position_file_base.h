///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6-dirty)
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
#include <wx/choice.h>
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
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_formatLabel;
		wxChoice* m_formatCtrl;
		wxStaticText* m_unitsLabel;
		wxChoice* m_unitsCtrl;
		wxCheckBox* m_onlySMD;
		wxCheckBox* m_excludeTH;
		wxCheckBox* m_excludeDNP;
		wxCheckBox* m_excludeBOM;
		wxCheckBox* m_cbIncludeBoardEdge;
		wxCheckBox* m_useDrillPlaceOrigin;
		wxCheckBox* m_negateXcb;
		wxCheckBox* m_singleFile;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateUIFileOpt( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateUIUnits( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateUIOnlySMD( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateUIExcludeTH( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateUIincludeBoardEdge( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateUInegXcoord( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onGenerate( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GEN_FOOTPRINT_POSITION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Generate Placement Files"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GEN_FOOTPRINT_POSITION_BASE();

};

