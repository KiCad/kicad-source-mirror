///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Jul 15 2020)
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
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_3D_VIEW_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_3D_VIEW_OPTIONS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_panelDspOpt;
		wxStaticBitmap* m_bitmapRealisticMode;
		wxCheckBox* m_checkBoxRealisticMode;
		wxStaticBitmap* m_bitmapBoardBody;
		wxCheckBox* m_checkBoxBoardBody;
		wxStaticBitmap* m_bitmapAreas;
		wxCheckBox* m_checkBoxAreas;
		wxStaticBitmap* m_bitmapSubtractMaskFromSilk;
		wxCheckBox* m_checkBoxSubtractMaskFromSilk;
		wxStaticBitmap* m_bitmapClipSilkOnViaAnnulus;
		wxCheckBox* m_checkBoxClipSilkOnViaAnnulus;
		wxStaticBitmap* m_bitmap3DshapesTH;
		wxCheckBox* m_checkBox3DshapesTH;
		wxStaticBitmap* m_bitmap3DshapesSMD;
		wxCheckBox* m_checkBox3DshapesSMD;
		wxStaticBitmap* m_bitmap3DshapesVirtual;
		wxCheckBox* m_checkBox3DshapesVirtual;
		wxStaticLine* m_staticlineVertical;
		wxStaticBitmap* m_bitmapSilkscreen;
		wxCheckBox* m_checkBoxSilkscreen;
		wxStaticBitmap* m_bitmapSolderMask;
		wxCheckBox* m_checkBoxSolderMask;
		wxStaticBitmap* m_bitmapSolderPaste;
		wxCheckBox* m_checkBoxSolderpaste;
		wxStaticBitmap* m_bitmapAdhesive;
		wxCheckBox* m_checkBoxAdhesive;
		wxStaticBitmap* m_bitmapComments;
		wxCheckBox* m_checkBoxComments;
		wxStaticBitmap* m_bitmapECO;
		wxCheckBox* m_checkBoxECO;
		wxStaticText* m_staticTextRotAngle;
		wxSpinCtrlDouble* m_spinCtrlRotationAngle;
		wxStaticText* m_staticTextRotAngleUnits;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkBoxEnableAnimation;
		wxStaticText* m_staticAnimationSpeed;
		wxSlider* m_sliderAnimationSpeed;
		wxPanel* m_panelOpenGL;
		wxStaticBitmap* m_bitmapBoundingBoxes;
		wxCheckBox* m_checkBoxBoundingBoxes;
		wxStaticBitmap* m_bitmapCuThickness;
		wxCheckBox* m_checkBoxCuThickness;
		wxChoice* m_choiceAntiAliasing;
		wxCheckBox* m_checkBoxDisableAAMove;
		wxCheckBox* m_checkBoxDisableMoveThickness;
		wxCheckBox* m_checkBoxDisableMoveVias;
		wxCheckBox* m_checkBoxDisableMoveHoles;
		wxPanel* m_panelRaytracing;
		wxCheckBox* m_checkBoxRaytracing_renderShadows;
		wxCheckBox* m_checkBoxRaytracing_proceduralTextures;
		wxCheckBox* m_checkBoxRaytracing_addFloor;
		wxCheckBox* m_checkBoxRaytracing_showRefractions;
		wxCheckBox* m_checkBoxRaytracing_showReflections;
		wxCheckBox* m_checkBoxRaytracing_antiAliasing;
		wxCheckBox* m_checkBoxRaytracing_postProcessing;
		wxStaticLine* m_staticlineH;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnCheckRealisticMode( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCheckEnableAnimation( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_3D_VIEW_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("3D Display Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 659,567 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_3D_VIEW_OPTIONS_BASE();

};

