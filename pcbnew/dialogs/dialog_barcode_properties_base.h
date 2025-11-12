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
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/bmpcbox.h>
#include <wx/combobox.h>
#include <wx/radiobox.h>
#include <wx/gbsizer.h>
#include <pcb_base_frame.h>
#include <pcb_draw_panel_gal.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BARCODE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BARCODE_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_DIALOG_EDIT_PAD = 10000,
		};

		wxStaticText* m_staticText28;
		wxTextCtrl* m_textInput;
		wxCheckBox* m_cbLocked;
		wxCheckBox* m_cbKnockout;
		wxCheckBox* m_cbShowText;
		wxBoxSizer* m_middleBoxSizer;
		wxStaticText* m_layerLabel;
		PCB_LAYER_BOX_SELECTOR* m_cbLayer;
		wxStaticText* m_posXLabel;
		wxTextCtrl* m_posXCtrl;
		wxStaticText* m_posXUnits;
		wxStaticText* m_posYLabel;
		wxTextCtrl* m_posYCtrl;
		wxStaticText* m_posYUnits;
		wxStaticText* m_sizeXLabel;
		wxTextCtrl* m_sizeXCtrl;
		wxStaticText* m_sizeXUnits;
		wxStaticText* m_sizeYLabel;
		wxTextCtrl* m_sizeYCtrl;
		wxStaticText* m_sizeYUnits;
		wxStaticText* m_orientationLabel;
		wxComboBox* m_orientationCtrl;
		wxStaticText* m_orientationUnits;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_marginXLabel;
		wxTextCtrl* m_marginXCtrl;
		wxStaticText* m_marginXUnits;
		wxStaticText* m_marginYLabel;
		wxTextCtrl* m_marginYCtrl;
		wxStaticText* m_marginYUnits;
		wxRadioBox* m_barcode;
		wxRadioBox* m_errorCorrection;
		PCB_DRAW_PANEL_GAL* m_panelShowBarcodeGal;
		KIGFX::GAL_DISPLAY_OPTIONS m_galOptions;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnTextValueChanged( wxKeyEvent& event ) { event.Skip(); }
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_BARCODE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Barcode Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_BARCODE_PROPERTIES_BASE();

};

