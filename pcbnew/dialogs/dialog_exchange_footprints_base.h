///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXCHANGE_FOOTPRINTS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXCHANGE_FOOTPRINTS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxGridBagSizer* m_upperSizer;
		wxRadioButton* m_matchAll;
		wxRadioButton* m_matchSelected;
		wxRadioButton* m_matchSpecifiedRef;
		wxTextCtrl* m_specifiedRef;
		wxRadioButton* m_matchSpecifiedValue;
		wxTextCtrl* m_specifiedValue;
		wxRadioButton* m_matchSpecifiedID;
		wxTextCtrl* m_specifiedID;
		STD_BITMAP_BUTTON* m_specifiedIDBrowseButton;
		wxBoxSizer* m_changeSizer;
		wxTextCtrl* m_newID;
		STD_BITMAP_BUTTON* m_newIDBrowseButton;
		wxStaticBoxSizer* m_updateOptionsSizer;
		wxCheckBox* m_removeExtraBox;
		wxCheckBox* m_resetTextItemLayers;
		wxCheckBox* m_resetTextItemEffects;
		wxCheckBox* m_resetTextItemPositions;
		wxCheckBox* m_resetTextItemContent;
		wxButton* m_checkAll;
		wxCheckBox* m_resetFabricationAttrs;
		wxCheckBox* m_resetClearanceOverrides;
		wxCheckBox* m_reset3DModels;
		wxButton* m_uncheckAll;
		WX_HTML_REPORT_PANEL* m_MessageWindow;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void updateMatchModeRadioButtons( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnMatchAllClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchSelectedClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchRefClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchValueClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMatchIDClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ViewAndSelectFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCheckAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUncheckAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXCHANGE_FOOTPRINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Update Footprints from Library"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXCHANGE_FOOTPRINTS_BASE();

};

