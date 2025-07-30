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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statline.h>
#include <wx/hyperlink.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_FIND_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_FIND_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxComboBox* m_comboFind;
		wxStaticText* m_staticReplace;
		wxComboBox* m_comboReplace;
		wxStaticText* m_staticDirection;
		wxRadioButton* m_radioForward;
		wxRadioButton* m_radioBackward;
		wxCheckBox* m_checkMatchCase;
		wxCheckBox* m_checkWholeWord;
		wxCheckBox* m_checkRegexMatch;
		wxCheckBox* m_cbSearchPins;
		wxCheckBox* m_cbSearchHiddenFields;
		wxCheckBox* m_cbCurrentSheetOnly;
		wxCheckBox* m_cbSelectedOnly;
		wxCheckBox* m_cbReplaceReferences;
		wxCheckBox* m_cbSearchNetNames;
		wxButton* m_buttonFind;
		wxButton* m_buttonReplace;
		wxButton* m_buttonReplaceAll;
		wxButton* m_buttonCancel;
		wxStaticLine* m_staticline1;
		wxHyperlinkCtrl* m_searchPanelLink;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnIdle( wxIdleEvent& event ) { event.Skip(); }
		virtual void OnSearchForSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchForText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchForEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateDrcUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnReplaceWithSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReplaceWithText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReplaceWithEnter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOptions( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFind( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReplace( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnUpdateReplaceUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnUpdateReplaceAllUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void onShowSearchPanel( wxHyperlinkEvent& event ) { event.Skip(); }


	public:

		DIALOG_SCH_FIND_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Find"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SCH_FIND_BASE();

};

