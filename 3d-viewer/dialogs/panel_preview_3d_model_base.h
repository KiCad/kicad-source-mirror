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
class STD_BITMAP_BUTTON;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <widgets/text_ctrl_eval.h>
#include <wx/spinbutt.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/slider.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PREVIEW_3D_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PREVIEW_3D_MODEL_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText1;
		TEXT_CTRL_EVAL* xscale;
		wxSpinButton* m_spinXscale;
		wxStaticText* m_staticText2;
		TEXT_CTRL_EVAL* yscale;
		wxSpinButton* m_spinYscale;
		wxStaticText* m_staticText3;
		TEXT_CTRL_EVAL* zscale;
		wxSpinButton* m_spinZscale;
		wxStaticText* m_staticText11;
		TEXT_CTRL_EVAL* xrot;
		wxSpinButton* m_spinXrot;
		wxStaticText* m_staticText21;
		TEXT_CTRL_EVAL* yrot;
		wxSpinButton* m_spinYrot;
		wxStaticText* m_staticText31;
		TEXT_CTRL_EVAL* zrot;
		wxSpinButton* m_spinZrot;
		wxStaticText* m_staticText12;
		TEXT_CTRL_EVAL* xoff;
		wxSpinButton* m_spinXoffset;
		wxStaticText* m_staticText22;
		wxSpinButton* m_spinYoffset;
		wxStaticText* m_staticText32;
		TEXT_CTRL_EVAL* zoff;
		wxSpinButton* m_spinZoffset;
		wxSlider* m_opacity;
		wxStaticText* m_previewLabel;
		wxBoxSizer* m_SizerPanelView;
		STD_BITMAP_BUTTON* m_bpvISO;
		STD_BITMAP_BUTTON* m_bpvBodyStyle;
		STD_BITMAP_BUTTON* m_bpvLeft;
		STD_BITMAP_BUTTON* m_bpvRight;
		STD_BITMAP_BUTTON* m_bpvFront;
		STD_BITMAP_BUTTON* m_bpvBack;
		STD_BITMAP_BUTTON* m_bpvTop;
		STD_BITMAP_BUTTON* m_bpvBottom;
		STD_BITMAP_BUTTON* m_bpUpdate;
		STD_BITMAP_BUTTON* m_bpSettings;

		// Virtual event handlers, override them in your derived class
		virtual void onMouseWheelScale( wxMouseEvent& event ) { event.Skip(); }
		virtual void updateOrientation( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDecrementScale( wxSpinEvent& event ) { event.Skip(); }
		virtual void onIncrementScale( wxSpinEvent& event ) { event.Skip(); }
		virtual void onMouseWheelRot( wxMouseEvent& event ) { event.Skip(); }
		virtual void onDecrementRot( wxSpinEvent& event ) { event.Skip(); }
		virtual void onIncrementRot( wxSpinEvent& event ) { event.Skip(); }
		virtual void onMouseWheelOffset( wxMouseEvent& event ) { event.Skip(); }
		virtual void onDecrementOffset( wxSpinEvent& event ) { event.Skip(); }
		virtual void onIncrementOffset( wxSpinEvent& event ) { event.Skip(); }
		virtual void onOpacitySlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DISO( wxCommandEvent& event ) { event.Skip(); }
		virtual void setBodyStyleView( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DLeft( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DRight( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DFront( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DBack( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DTop( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DBottom( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DUpdate( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DSettings( wxCommandEvent& event ) { event.Skip(); }


	public:
		TEXT_CTRL_EVAL* yoff;

		PANEL_PREVIEW_3D_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PREVIEW_3D_MODEL_BASE();

};

