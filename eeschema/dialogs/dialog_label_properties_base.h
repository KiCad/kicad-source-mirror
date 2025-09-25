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
class BITMAP_BUTTON;
class COLOR_SWATCH;
class FILTER_COMBOBOX;
class FONT_CHOICE;
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/gbsizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LABEL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LABEL_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_VALUESINGLE = 6000,
		};

		wxFlexGridSizer* m_textEntrySizer;
		wxStaticText* m_labelSingleLine;
		wxTextCtrl* m_valueSingleLine;
		wxStaticText* m_labelCombo;
		FILTER_COMBOBOX* m_valueCombo;
		wxStaticText* m_labelMultiLine;
		wxTextCtrl* m_valueMultiLine;
		wxCheckBox* m_cbMultiLine;
		wxHyperlinkCtrl* m_syntaxHelp;
		WX_GRID* m_grid;
		STD_BITMAP_BUTTON* m_bpAdd;
		STD_BITMAP_BUTTON* m_bpMoveUp;
		STD_BITMAP_BUTTON* m_bpMoveDown;
		STD_BITMAP_BUTTON* m_bpDelete;
		wxStaticBoxSizer* m_shapeSizer;
		wxRadioButton* m_input;
		wxRadioButton* m_output;
		wxRadioButton* m_bidirectional;
		wxRadioButton* m_triState;
		wxRadioButton* m_passive;
		wxRadioButton* m_dot;
		wxRadioButton* m_circle;
		wxRadioButton* m_diamond;
		wxRadioButton* m_rectangle;
		wxGridBagSizer* m_formattingGB;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		wxBoxSizer* m_iconBar;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_spin0;
		BITMAP_BUTTON* m_spin1;
		BITMAP_BUTTON* m_spin2;
		BITMAP_BUTTON* m_spin3;
		wxCheckBox* m_autoRotate;
		BITMAP_BUTTON* m_separator3;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_textColorLabel;
		wxPanel* m_panelBorderColor1;
		COLOR_SWATCH* m_textColorSwatch;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnValueCharHook( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnEnterKey( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCBValueCharHook( wxKeyEvent& event ) { event.Skip(); }
		virtual void onMultiLabelCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFormattingHelp( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LABEL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Label Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_LABEL_PROPERTIES_BASE();

};

