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
#include "dialog_shim.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/slider.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COLOR_PICKER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COLOR_PICKER_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_panelFreeColors;
		wxStaticBitmap* m_RgbBitmap;
		wxStaticText* m_staticTextR;
		wxStaticText* m_staticTextG;
		wxStaticText* m_staticTextB;
		wxSpinCtrl* m_spinCtrlRed;
		wxSpinCtrl* m_spinCtrlGreen;
		wxSpinCtrl* m_spinCtrlBlue;
		wxStaticBitmap* m_HsvBitmap;
		wxStaticText* m_staticTextHue;
		wxStaticText* m_staticTextSat;
		wxSpinCtrl* m_spinCtrlHue;
		wxSpinCtrl* m_spinCtrlSaturation;
		wxStaticText* m_staticTextBright;
		wxSlider* m_sliderBrightness;
		wxPanel* m_panelDefinedColors;
		wxBoxSizer* m_SizerDefinedColors;
		wxFlexGridSizer* m_fgridColor;
		wxBoxSizer* m_SizerTransparency;
		wxStaticText* m_opacityLabel;
		wxSlider* m_sliderTransparency;
		wxStaticText* m_staticTextOldColor;
		wxStaticBitmap* m_OldColorRect;
		wxStaticBitmap* m_NewColorRect;
		wxTextCtrl* m_colorValue;
		wxButton* m_resetToDefault;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onRGBMouseClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onRGBMouseDrag( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnChangeEditRed( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnChangeEditGreen( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnChangeEditBlue( wxSpinEvent& event ) { event.Skip(); }
		virtual void onHSVMouseClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onHSVMouseDrag( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnChangeEditHue( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnChangeEditSat( wxSpinEvent& event ) { event.Skip(); }
		virtual void OnChangeBrightness( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnChangeAlpha( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnColorValueText( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnResetButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_COLOR_PICKER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Color Picker"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_COLOR_PICKER_BASE();

};

