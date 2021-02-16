///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/bmpcbox.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_SPECIFIED_NET_TO_NETCLASS_VALUES 1000
#define ID_ALL_TRACKS_VIAS 1001

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxCheckBox* m_references;
		wxCheckBox* m_values;
		wxCheckBox* m_otherFields;
		wxCheckBox* m_footprintGraphics;
		wxCheckBox* m_boardGraphics;
		wxCheckBox* m_boardText;
		wxCheckBox* m_layerFilterOpt;
		PCB_LAYER_BOX_SELECTOR* m_layerFilter;
		wxCheckBox* m_referenceFilterOpt;
		wxTextCtrl* m_referenceFilter;
		wxCheckBox* m_footprintFilterOpt;
		wxTextCtrl* m_footprintFilter;
		wxCheckBox* m_selectedItemsFilter;
		wxRadioButton* m_setToSpecifiedValues;
		wxPanel* m_specifiedValues;
		wxStaticText* m_LayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_LayerCtrl;
		wxCheckBox* m_Visible;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_LineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_SizeXlabel;
		wxTextCtrl* m_SizeXCtrl;
		wxStaticText* m_SizeXunit;
		wxCheckBox* m_Italic;
		wxStaticText* m_SizeYlabel;
		wxTextCtrl* m_SizeYCtrl;
		wxStaticText* m_SizeYunit;
		wxCheckBox* m_keepUpright;
		wxStaticText* m_ThicknessLabel;
		wxTextCtrl* m_ThicknessCtrl;
		wxStaticText* m_ThicknessUnit;
		wxRadioButton* m_setToLayerDefaults;
		wxGrid* m_grid;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLayerFilterSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReferenceFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFootprintFilterText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeNetclassGrid( wxSizeEvent& event ) { event.Skip(); }


	public:

		DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Edit Text and Graphic Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GLOBAL_EDIT_TEXT_AND_GRAPHICS_BASE();

};

