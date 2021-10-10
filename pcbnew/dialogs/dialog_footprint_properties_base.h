///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class TEXT_CTRL_EVAL;
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
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/gbsizer.h>
#include <wx/radiobox.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_NOTEBOOK 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINT_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINT_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:
		wxBoxSizer* m_GeneralBoxSizer;
		wxBoxSizer* bSizerLeft;

	protected:
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelGeneral;
		WX_GRID* m_itemsGrid;
		wxBitmapButton* m_bpAdd;
		wxBitmapButton* m_bpDelete;
		wxStaticText* m_XPosLabel;
		wxTextCtrl* m_ModPositionX;
		wxStaticText* m_XPosUnit;
		wxStaticText* m_YPosLabel;
		wxTextCtrl* m_ModPositionY;
		wxStaticText* m_YPosUnit;
		wxStaticText* m_BoardSideLabel;
		wxChoice* m_BoardSideCtrl;
		wxRadioButton* m_Orient0;
		wxRadioButton* m_Orient90;
		wxRadioButton* m_Orient270;
		wxRadioButton* m_Orient180;
		wxRadioButton* m_OrientOther;
		wxTextCtrl* m_OrientValueCtrl;
		wxRadioBox* m_AutoPlaceCtrl;
		wxStaticBoxSizer* m_sizerAP;
		wxBoxSizer* m_sizerAllow90;
		wxStaticText* m_allow90Label;
		wxSlider* m_CostRot90Ctrl;
		wxBoxSizer* m_sizerAllow180;
		wxStaticText* m_allow180Label;
		wxSlider* m_CostRot180Ctrl;
		wxButton* m_buttonUpdate;
		wxButton* m_buttonExchange;
		wxButton* m_buttonModuleEditor;
		wxButton* m_button5;
		wxStaticText* m_componentTypeLabel;
		wxChoice* m_componentType;
		wxCheckBox* m_boardOnly;
		wxCheckBox* m_excludeFromPosFiles;
		wxCheckBox* m_excludeFromBOM;
		wxPanel* m_PanelClearances;
		wxStaticText* m_staticTextInfo;
		wxStaticText* m_staticTextInfoValPos;
		wxStaticText* m_staticTextInfoValNeg;
		wxStaticText* m_NetClearanceLabel;
		wxTextCtrl* m_NetClearanceCtrl;
		wxStaticText* m_NetClearanceUnits;
		wxStaticText* m_SolderMaskMarginLabel;
		wxTextCtrl* m_SolderMaskMarginCtrl;
		wxStaticText* m_SolderMaskMarginUnits;
		wxStaticText* m_SolderPasteMarginLabel;
		wxTextCtrl* m_SolderPasteMarginCtrl;
		wxStaticText* m_SolderPasteMarginUnits;
		wxStaticText* m_PasteMarginRatioLabel;
		TEXT_CTRL_EVAL* m_PasteMarginRatioCtrl;
		wxStaticText* m_PasteMarginRatioUnits;
		wxStaticText* m_staticTextInfoCopper;
		wxStaticText* m_staticTextInfoPaste;
		wxStaticText* m_staticText16;
		wxChoice* m_ZoneConnectionChoice;
		wxStaticText* m_libraryIDLabel;
		wxTextCtrl* m_tcLibraryID;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnPageChange( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }
		virtual void FootprintOrientEvent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOtherOrientation( wxCommandEvent& event ) { event.Skip(); }
		virtual void UpdateFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void ChangeFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void EditFootprint( wxCommandEvent& event ) { event.Skip(); }
		virtual void EditLibraryFootprint( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_FOOTPRINT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FOOTPRINT_PROPERTIES_BASE();

};

