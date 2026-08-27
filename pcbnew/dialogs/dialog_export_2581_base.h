///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
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
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_2581_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_2581_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerTop;
		wxStaticText* m_lblBrdFile;
		wxTextCtrl* m_outputFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_fileFormatLabel;
		wxStaticLine* m_staticline1;
		wxStaticText* m_lblUnits;
		wxChoice* m_choiceUnits;
		wxStaticText* m_lblPrecision;
		wxSpinCtrl* m_precision;
		wxStaticText* m_lblVersion;
		wxChoice* m_versionChoice;
		wxCheckBox* m_cbCompress;
		wxStaticText* m_columnsLabel;
		wxStaticLine* m_staticline2;
		wxStaticText* m_lblDataSet;
		wxChoice* m_choiceDataSet;
		wxStaticText* m_lblNetNames;
		wxChoice* m_choiceNetNames;
		wxStaticText* m_lblRefDes;
		wxChoice* m_choiceRefDes;
		wxStaticText* m_lblIncludes;
		wxBoxSizer* bSizerContentButtons;
		wxButton* m_btnCustomize;
		wxButton* m_btnBomFields;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCompressCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDataSetChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCustomizeClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBomFieldsClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOKClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_2581_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export IPC-2581"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_2581_BASE();

};

