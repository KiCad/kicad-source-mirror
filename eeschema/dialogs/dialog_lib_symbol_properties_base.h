///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jun  3 2020)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>
#include <wx/listbox.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_LIBEDIT_NOTEBOOK 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_LIB_SYMBOL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_LIB_SYMBOL_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelBasic;
		WX_GRID* m_grid;
		wxBoxSizer* bButtonSize;
		wxBitmapButton* m_bpAdd;
		wxBitmapButton* m_bpMoveUp;
		wxBitmapButton* m_bpMoveDown;
		wxBitmapButton* m_bpDelete;
		wxTextCtrl* m_SymbolNameCtrl;
		wxTextCtrl* m_DescCtrl;
		wxStaticText* staticKeywordsLabel;
		wxTextCtrl* m_KeywordCtrl;
		wxStaticText* m_inheritsStaticText;
		wxComboBox* m_inheritanceSelectCombo;
		wxBoxSizer* bSizerLowerBasicPanel;
		wxCheckBox* m_AsConvertButt;
		wxCheckBox* m_OptionPower;
		wxCheckBox* m_excludeFromBomCheckBox;
		wxCheckBox* m_excludeFromBoardCheckBox;
		wxStaticText* m_staticTextNbUnits;
		wxSpinCtrl* m_SelNumberOfUnits;
		wxCheckBox* m_OptionPartsInterchangeable;
		wxCheckBox* m_ShowPinNumButt;
		wxCheckBox* m_ShowPinNameButt;
		wxCheckBox* m_PinsNameInsideButt;
		wxStaticText* m_nameOffsetLabel;
		wxTextCtrl* m_nameOffsetCtrl;
		wxStaticText* m_nameOffsetUnits;
		wxPanel* m_PanelFootprintFilter;
		wxStaticText* m_staticTextFootprints;
		wxListBox* m_FootprintFilterListBox;
		wxBitmapButton* m_addFilterButton;
		wxBitmapButton* m_editFilterButton;
		wxBitmapButton* m_deleteFilterButton;
		wxButton* m_spiceFieldsButton;
		wxStdDialogButtonSizer* m_stdSizerButton;
		wxButton* m_stdSizerButtonOK;
		wxButton* m_stdSizerButtonCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSymbolNameKillFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSymbolNameText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPowerCheckBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterDClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnEditFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteFootprintFilter( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditSpiceModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_LIB_SYMBOL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = ID_LIBEDIT_NOTEBOOK, const wxString& title = _("Library Symbol Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_LIB_SYMBOL_PROPERTIES_BASE();

};

