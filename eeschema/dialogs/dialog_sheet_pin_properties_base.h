///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.1.0-0-g733bf3d)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class COLOR_SWATCH;
class FONT_CHOICE;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/gbsizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SHEET_PIN_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SHEET_PIN_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxComboBox* m_comboName;
		wxHyperlinkCtrl* m_hyperlink1;
		wxStaticBoxSizer* m_shapeSizer;
		wxRadioButton* m_input;
		wxRadioButton* m_output;
		wxRadioButton* m_bidirectional;
		wxRadioButton* m_triState;
		wxRadioButton* m_passive;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator2;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_textColorLabel;
		wxPanel* m_panelBorderColor1;
		COLOR_SWATCH* m_textColorSwatch;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onComboBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSyntaxHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void onOKButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SHEET_PIN_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Sheet Pin Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_SHEET_PIN_PROPERTIES_BASE();

};

