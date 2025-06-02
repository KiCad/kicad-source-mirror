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
class FONT_CHOICE;
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/bmpcbox.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/gbsizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEXTBOX_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEXTBOX_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bMainSizer;
		wxBoxSizer* m_MultiLineSizer;
		wxStyledTextCtrl* m_MultiLineText;
		wxCheckBox* m_cbLocked;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxStaticText* m_fontLabel;
		FONT_CHOICE* m_fontCtrl;
		BITMAP_BUTTON* m_separator0;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_hAlignLeft;
		BITMAP_BUTTON* m_hAlignCenter;
		BITMAP_BUTTON* m_hAlignRight;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_vAlignTop;
		BITMAP_BUTTON* m_vAlignCenter;
		BITMAP_BUTTON* m_vAlignBottom;
		BITMAP_BUTTON* m_separator3;
		BITMAP_BUTTON* m_mirrored;
		BITMAP_BUTTON* m_separator4;
		wxStaticText* m_SizeXLabel;
		wxTextCtrl* m_SizeXCtrl;
		wxStaticText* m_SizeXUnits;
		wxCheckBox* m_borderCheckbox;
		wxStaticText* m_SizeYLabel;
		wxTextCtrl* m_SizeYCtrl;
		wxStaticText* m_SizeYUnits;
		wxStaticText* m_ThicknessLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_ThicknessUnits;
		BITMAP_BUTTON* m_autoTextThickness;
		wxStaticText* m_OrientLabel;
		wxComboBox* m_OrientCtrl;
		wxStaticText* m_borderWidthLabel;
		wxTextCtrl* m_borderWidthCtrl;
		wxStaticText* m_borderWidthUnits;
		wxStaticText* m_borderStyleLabel;
		wxBitmapComboBox* m_borderStyleCombo;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void onFontSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBoldToggle( wxCommandEvent& event ) { event.Skip(); }
		virtual void onHalignButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onValignButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTextSize( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBorderChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onThickness( wxCommandEvent& event ) { event.Skip(); }
		virtual void onAutoTextThickness( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TEXTBOX_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Box Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );

		~DIALOG_TEXTBOX_PROPERTIES_BASE();

};

