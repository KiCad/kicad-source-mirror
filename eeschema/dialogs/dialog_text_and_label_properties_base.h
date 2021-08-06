///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
#include <wx/stc/stc.h>
#include <wx/combobox.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_VALUESINGLE = 1000,
			wxID_SIZE
		};

		wxFlexGridSizer* m_textEntrySizer;
		wxStaticText* m_labelSingleLine;
		wxTextCtrl* m_valueSingleLine;
		wxStaticText* m_labelMultiLine;
		wxStyledTextCtrl* m_valueMultiLine;
		wxStaticText* m_labelCombo;
		wxComboBox* m_valueCombo;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxHyperlinkCtrl* m_formattingHelp;
		wxStaticText* m_note1;
		wxStaticText* m_note2;
		wxRadioBox* m_TextOrient;
		wxRadioBox* m_TextStyle;
		wxRadioBox* m_TextShape;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnEnterKey( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFormattingHelp( wxHyperlinkEvent& event ) { event.Skip(); }


	public:

		DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_TEXT_AND_LABEL_PROPERTIES_BASE();

};

