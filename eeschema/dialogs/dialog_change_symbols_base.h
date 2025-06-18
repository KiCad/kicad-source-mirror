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
#include <wx/radiobut.h>
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
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/checklst.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CHANGE_SYMBOLS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CHANGE_SYMBOLS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxGridBagSizer* m_matchSizer;
		wxRadioButton* m_matchAll;
		wxRadioButton* m_matchBySelection;
		wxRadioButton* m_matchByReference;
		wxTextCtrl* m_specifiedReference;
		wxRadioButton* m_matchByValue;
		wxTextCtrl* m_specifiedValue;
		wxRadioButton* m_matchById;
		wxTextCtrl* m_specifiedId;
		STD_BITMAP_BUTTON* m_matchIdBrowserButton;
		wxStaticLine* m_staticline1;
		wxBoxSizer* m_newIdSizer;
		wxTextCtrl* m_newId;
		STD_BITMAP_BUTTON* m_newIdBrowserButton;
		wxStaticBoxSizer* m_updateFieldsSizer;
		wxCheckListBox* m_fieldsBox;
		wxButton* m_selAllBtn;
		wxButton* m_selNoneBtn;
		wxStaticBoxSizer* m_updateOptionsSizer;
		wxCheckBox* m_removeExtraBox;
		wxCheckBox* m_resetEmptyFields;
		wxCheckBox* m_resetFieldText;
		wxCheckBox* m_resetFieldVisibilities;
		wxCheckBox* m_resetFieldEffects;
		wxCheckBox* m_resetFieldPositions;
		wxButton* m_checkAll;
		wxCheckBox* m_resetPinTextVisibility;
		wxCheckBox* m_resetAlternatePin;
		wxCheckBox* m_resetAttributes;
		wxCheckBox* m_resetCustomPower;
		wxButton* m_uncheckAll;
		WX_HTML_REPORT_PANEL* m_messagePanel;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onMatchByAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMatchBySelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMatchByReference( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMatchTextKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onMatchByValue( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMatchById( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMatchIDKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void launchMatchIdSymbolBrowser( wxCommandEvent& event ) { event.Skip(); }
		virtual void onNewLibIDKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void launchNewIdSymbolBrowser( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSelectAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSelectNone( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUncheckAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOkButtonClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_CHANGE_SYMBOLS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Update Symbols from Library"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_CHANGE_SYMBOLS_BASE();

};

