///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;
class WX_INFOBAR;

#include "widgets/wx_grid.h"
#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/bmpcbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GENERATED_TABLE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GENERATED_TABLE_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxNotebook* m_notebook;
		wxPanel* m_generalPanel;
		wxStaticText* m_layerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxStaticText* m_unitsLabel;
		wxChoice* m_unitsCtrl;
		wxStaticText* m_precisionLabel;
		wxSpinCtrl* m_precisionCtrl;
		wxCheckBox* m_cbLocked;
		wxPanel* m_columnsPanel;
		WX_GRID* m_columnGrid;
		wxButton* m_moveUpButton;
		wxButton* m_moveDownButton;
		wxButton* m_resetColumnsButton;
		wxBoxSizer* m_optionsSizer;
		wxPanel* m_appearancePanel;
		wxCheckBox* m_borderCheckbox;
		wxCheckBox* m_headerBorder;
		wxCheckBox* m_rowSeparators;
		wxCheckBox* m_colSeparators;
		wxStaticText* m_borderWidthLabel;
		wxTextCtrl* m_borderWidthCtrl;
		wxStaticText* m_borderWidthUnits;
		wxStaticText* m_separatorsWidthLabel;
		wxTextCtrl* m_separatorsWidthCtrl;
		wxStaticText* m_separatorsWidthUnits;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void onResetColumns( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBorderChecked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_GENERATED_TABLE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Table Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_GENERATED_TABLE_PROPERTIES_BASE();

};

