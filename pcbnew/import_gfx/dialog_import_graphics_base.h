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
class STD_BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/statbmp.h>
#include <wx/panel.h>
#include <wx/statline.h>
#include <wx/valtext.h>
#include <wx/radiobut.h>
#include <wx/bmpcbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_IMPORT_GRAPHICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_IMPORT_GRAPHICS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextFile;
		wxTextCtrl* m_textCtrlFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_importScaleLabel;
		wxTextCtrl* m_importScaleCtrl;
		wxStaticText* m_importWidthLabel;
		wxTextCtrl* m_importWidthCtrl;
		wxStaticText* m_importWidthUnits;
		wxStaticText* m_importHeightLabel;
		wxTextCtrl* m_importHeightCtrl;
		wxStaticText* m_importHeightUnits;
		wxCheckBox* m_aspectRatioCheckbox;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_dxfUnitsLabel;
		wxChoice* m_dxfUnitsChoice;
		wxPanel* m_imagePanel;
		wxStaticText* m_thresholdLabel;
		wxSlider* m_thresholdSlider;
		wxCheckBox* m_cbInvertColors;
		wxStaticBitmap* m_previewBitmap;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_placeAtCheckbox;
		wxStaticText* m_xLabel;
		wxTextCtrl* m_xCtrl;
		wxStaticText* m_yLabel;
		wxTextCtrl* m_yCtrl;
		wxStaticText* m_yUnits;
		wxStaticText* m_originLabel;
		wxChoice* m_originCtrl;
		wxRadioButton* m_radioBtnCurrentLayer;
		wxRadioButton* m_radioBtnMapLayers;
		wxRadioButton* m_radioBtnSingleLayer;
		PCB_LAYER_BOX_SELECTOR* m_SelLayerBox;
		wxCheckBox* m_cbGroupItems;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_rbFixDiscontinuities;
		wxStaticText* m_toleranceLabel;
		wxTextCtrl* m_toleranceCtrl;
		wxStaticText* m_toleranceUnits;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onFilenameChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBrowseFiles( wxCommandEvent& event ) { event.Skip(); }
		virtual void onScaleChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWidthChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onHeightChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onLockAspectChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onThresholdChanged( wxScrollEvent& event ) { event.Skip(); }
		virtual void onInvertChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_IMPORT_GRAPHICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Import Graphics File"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_IMPORT_GRAPHICS_BASE();

};

