///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stc/stc.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/bmpcbox.h>
#include <wx/gbsizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TABLECELL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TABLECELL_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxStyledTextCtrl* m_textCtrl;
		wxNotebook* m_notebook;
		wxPanel* m_tablePage;
		wxGridBagSizer* m_textEntrySizer;
		wxCheckBox* m_borderCheckbox;
		wxCheckBox* m_headerBorder;
		wxStaticText* m_borderWidthLabel;
		wxTextCtrl* m_borderWidthCtrl;
		wxStaticText* m_borderWidthUnits;
		wxStaticText* m_borderColorLabel;
		wxPanel* m_panelBorderColor;
		COLOR_SWATCH* m_borderColorSwatch;
		wxStaticText* m_borderStyleLabel;
		wxBitmapComboBox* m_borderStyleCombo;
		wxCheckBox* m_rowSeparators;
		wxCheckBox* m_colSeparators;
		wxStaticText* m_separatorsWidthLabel;
		wxTextCtrl* m_separatorsWidthCtrl;
		wxStaticText* m_separatorsWidthUnits;
		wxStaticText* m_separatorsColorLabel;
		wxPanel* m_panelSeparatorsColor;
		COLOR_SWATCH* m_separatorsColorSwatch;
		wxStaticText* m_separatorsStyleLabel;
		wxBitmapComboBox* m_separatorsStyleCombo;
		wxPanel* m_cellPage;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_hAlignLeft;
		BITMAP_BUTTON* m_hAlignCenter;
		BITMAP_BUTTON* m_hAlignRight;
		BITMAP_BUTTON* m_separator3;
		BITMAP_BUTTON* m_vAlignTop;
		BITMAP_BUTTON* m_vAlignCenter;
		BITMAP_BUTTON* m_vAlignBottom;
		BITMAP_BUTTON* m_separator4;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_textColorLabel;
		wxPanel* m_panelTextColor;
		COLOR_SWATCH* m_textColorSwatch;
		wxStaticText* m_fillColorLabel;
		wxPanel* m_panelFillColor;
		COLOR_SWATCH* m_fillColorSwatch;
		wxButton* m_applyButton;
		wxStaticText* m_hotkeyHint;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onBorderChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnApply( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Table Cell Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_TABLECELL_PROPERTIES_BASE();

};

