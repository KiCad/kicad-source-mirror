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
class STD_BITMAP_BUTTON;
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_EDIT_PIN_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_EDIT_PIN_TABLE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextPinNumbers;
		wxStaticText* m_pin_numbers_summary;
		wxStaticText* m_staticTextPinCount;
		wxStaticText* m_pin_count;
		wxStaticText* m_staticTextDuplicatePins;
		wxStaticText* m_duplicate_pins;
		WX_GRID* m_grid;
		wxCheckBox* m_cbFilterByUnit;
		wxChoice* m_unitFilter;
		wxCheckBox* m_cbFilterByBodyStyle;
		wxChoice* m_bodyStyleFilter;
		wxCheckBox* m_cbFilterSelected;
		wxRadioButton* m_rbExportAllPins;
		wxRadioButton* m_rbExportOnlyShownPins;
		wxButton* m_btnExportToFile;
		wxButton* m_btnExportToClipboard;
		wxRadioButton* m_rbReplaceAll;
		wxRadioButton* m_radioBtn1;
		wxButton* m_btnImportFromFile;
		wxButton* m_btnImportFromClipboard;
		STD_BITMAP_BUTTON* m_addButton;
		STD_BITMAP_BUTTON* m_deleteButton;
		BITMAP_BUTTON* m_divider1;
		wxCheckBox* m_cbGroup;
		wxButton* m_groupSelected;
		STD_BITMAP_BUTTON* m_refreshButton;
		wxStdDialogButtonSizer* m_Buttons;
		wxButton* m_ButtonsOK;
		wxButton* m_ButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) = 0;
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) = 0;
		virtual void OnCellEdited( wxGridEvent& event ) = 0;
		virtual void OnCellSelected( wxGridEvent& event ) = 0;
		virtual void OnSize( wxSizeEvent& event ) = 0;
		virtual void OnFilterCheckBox( wxCommandEvent& event ) = 0;
		virtual void OnFilterChoice( wxCommandEvent& event ) = 0;
		virtual void OnExportButtonClick( wxCommandEvent& event ) = 0;
		virtual void OnImportButtonClick( wxCommandEvent& event ) = 0;
		virtual void OnAddRow( wxCommandEvent& event ) = 0;
		virtual void OnDeleteRow( wxCommandEvent& event ) = 0;
		virtual void OnRebuildRows( wxCommandEvent& event ) = 0;
		virtual void OnGroupSelected( wxCommandEvent& event ) = 0;
		virtual void OnCancel( wxCommandEvent& event ) = 0;


	public:

		DIALOG_LIB_EDIT_PIN_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Pin Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_LIB_EDIT_PIN_TABLE_BASE();

};

