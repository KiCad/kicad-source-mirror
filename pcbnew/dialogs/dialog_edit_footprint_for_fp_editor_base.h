///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_EDIT_FOOTPRINT_FOR_FP_EDITOR_BASE_H__
#define __DIALOG_EDIT_FOOTPRINT_FOR_FP_EDITOR_BASE_H__

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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>
#include <wx/slider.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_NOTEBOOK 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FOOTPRINT_FP_EDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FOOTPRINT_FP_EDITOR_BASE : public DIALOG_SHIM
{
	private:
		wxBoxSizer* m_GeneralBoxSizer;
	
	protected:
		wxNotebook* m_NoteBook;
		wxPanel* m_PanelGeneral;
		WX_GRID* m_itemsGrid;
		wxBitmapButton* m_bpAdd;
		wxBitmapButton* m_bpDelete;
		wxTextCtrl* m_FootprintNameCtrl;
		wxTextCtrl* m_DocCtrl;
		wxStaticText* staticKeywordsLabel;
		wxTextCtrl* m_KeywordCtrl;
		wxRadioBox* m_AutoPlaceCtrl;
		wxStaticBoxSizer* m_sizerAP;
		wxBoxSizer* m_sizerAllow90;
		wxStaticText* m_allow90Label;
		wxSlider* m_CostRot90Ctrl;
		wxBoxSizer* m_sizerAllow180;
		wxStaticText* m_allow180Label;
		wxSlider* m_CostRot180Ctrl;
		wxRadioBox* m_AttributsCtrl;
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
		wxStaticText* m_staticTextRatio;
		TEXT_CTRL_EVAL* m_SolderPasteMarginRatioCtrl;
		wxStaticText* m_SolderPasteRatioMarginUnits;
		wxStaticText* m_staticTextInfoCopper;
		wxStaticText* m_staticText16;
		wxChoice* m_ZoneConnectionChoice;
		wxPanel* m_Panel3D;
		wxBoxSizer* bSizerMain3D;
		WX_GRID* m_modelsGrid;
		wxBitmapButton* m_button3DShapeAdd;
		wxBitmapButton* m_button3DShapeBrowse;
		wxBitmapButton* m_button3DShapeRemove;
		wxButton* m_buttonConfig3DPaths;
		wxBoxSizer* bLowerSizer3D;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteField( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFootprintNameText( wxCommandEvent& event ) { event.Skip(); }
		virtual void On3DModelCellChanged( wxGridEvent& event ) { event.Skip(); }
		virtual void On3DModelSelected( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAdd3DRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAdd3DModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemove3DModel( wxCommandEvent& event ) { event.Skip(); }
		virtual void Cfg3DPath( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_FOOTPRINT_FP_EDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Footprint Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_FOOTPRINT_FP_EDITOR_BASE();
	
};

#endif //__DIALOG_EDIT_FOOTPRINT_FOR_FP_EDITOR_BASE_H__
