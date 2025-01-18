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
#include <wx/scrolwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class BITMAP2CMP_PANEL_BASE
///////////////////////////////////////////////////////////////////////////////
class BITMAP2CMP_PANEL_BASE : public wxPanel
{
	private:
		wxStaticText* m_sizeLabel;

	protected:
		wxNotebook* m_Notebook;
		wxScrolledWindow* m_InitialPicturePanel;
		wxScrolledWindow* m_GreyscalePicturePanel;
		wxScrolledWindow* m_BNPicturePanel;
		wxStaticText* m_staticTextISize;
		wxStaticText* m_SizeXValue;
		wxStaticText* m_SizeYValue;
		wxStaticText* m_SizePixUnits;
		wxStaticText* m_staticTextDPI;
		wxStaticText* m_InputXValueDPI;
		wxStaticText* m_InputYValueDPI;
		wxStaticText* m_DPIUnit;
		wxStaticText* m_staticTextBPP;
		wxStaticText* m_BPPValue;
		wxStaticText* m_BPPunits;
		wxButton* m_buttonLoad;
		wxTextCtrl* m_UnitSizeX;
		wxTextCtrl* m_UnitSizeY;
		wxChoice* m_PixelUnit;
		wxCheckBox* m_aspectRatioCheckbox;
		wxStaticText* m_ThresholdText;
		wxSlider* m_sliderThreshold;
		wxCheckBox* m_checkNegative;
		wxRadioButton* m_rbSymbol;
		wxRadioButton* m_rbFootprint;
		wxStaticText* m_layerLabel;
		wxChoice* m_layerCtrl;
		wxRadioButton* m_rbPostscript;
		wxRadioButton* m_rbWorksheet;
		wxButton* m_buttonExportFile;
		wxButton* m_buttonExportClipboard;

		// Virtual event handlers, override them in your derived class
		virtual void OnPaintInit( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnPaintGreyscale( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnPaintBW( wxPaintEvent& event ) { event.Skip(); }
		virtual void OnLoadFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeChangeX( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeChangeY( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeUnitChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToggleAspectRatioLock( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnThresholdChange( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnNegativeClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFormatChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExportToFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnExportToClipboard( wxCommandEvent& event ) { event.Skip(); }


	public:

		BITMAP2CMP_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~BITMAP2CMP_PANEL_BASE();

};

