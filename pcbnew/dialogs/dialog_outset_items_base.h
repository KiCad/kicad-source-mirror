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
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/bmpcbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_OUTSET_ITEMS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_OUTSET_ITEMS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_outsetLabel;
		wxComboBox* m_outsetEntry;
		wxStaticText* m_outsetUnit;
		wxCheckBox* m_roundToGrid;
		wxCheckBox* m_roundCorners;
		wxStaticText* m_gridRoundingLabel;
		wxComboBox* m_gridRoundingEntry;
		wxStaticText* m_gridRoundingUnit;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_copyLayers;
		wxStaticText* m_layerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerSelectionCtrl;
		wxCheckBox* m_copyWidths;
		wxStaticText* m_lineWidthLabel;
		wxComboBox* m_lineWidthEntry;
		wxStaticText* m_lineWidthUnit;
		wxButton* m_layerDefaultBtn;
		wxStaticLine* m_staticline21;
		wxCheckBox* m_deleteSourceItems;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnRoundToGridChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCopyLayersChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLayerDefaultClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_OUTSET_ITEMS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Outset Items"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_OUTSET_ITEMS_BASE();

};

