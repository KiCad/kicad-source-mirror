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
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checklst.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_UPDATE_SYMBOL_FIELDS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_UPDATE_SYMBOL_FIELDS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxBoxSizer* m_newIdSizer;
		wxTextCtrl* m_parentSymbolReadOnly;
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
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onSelectAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSelectNone( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOkButtonClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_UPDATE_SYMBOL_FIELDS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Update Symbol Fields"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_UPDATE_SYMBOL_FIELDS_BASE();

};

