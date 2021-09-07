///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Aug 10 2021)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/color_swatch.h"
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_3D_RAYTRACING_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_3D_RAYTRACING_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxCheckBox* m_checkBoxRaytracing_proceduralTextures;
		wxCheckBox* m_checkBoxRaytracing_addFloor;
		wxCheckBox* m_checkBoxRaytracing_antiAliasing;
		wxCheckBox* m_checkBoxRaytracing_postProcessing;
		wxStaticText* m_staticText19;
		wxStaticText* m_staticText201;
		wxStaticText* m_staticText211;
		wxCheckBox* m_checkBoxRaytracing_renderShadows;
		wxSpinCtrl* m_spinCtrl_NrSamples_Shadows;
		wxSpinCtrlDouble* m_spinCtrlDouble_SpreadFactor_Shadows;
		wxCheckBox* m_checkBoxRaytracing_showReflections;
		wxSpinCtrl* m_spinCtrl_NrSamples_Reflections;
		wxSpinCtrlDouble* m_spinCtrlDouble_SpreadFactor_Reflections;
		wxSpinCtrl* m_spinCtrlRecursiveLevel_Reflections;
		wxCheckBox* m_checkBoxRaytracing_showRefractions;
		wxSpinCtrl* m_spinCtrl_NrSamples_Refractions;
		wxSpinCtrlDouble* m_spinCtrlDouble_SpreadFactor_Refractions;
		wxSpinCtrl* m_spinCtrlRecursiveLevel_Refractions;
		wxStaticText* m_staticText17;
		COLOR_SWATCH* m_colourPickerCameraLight;
		wxStaticText* m_staticText5;
		COLOR_SWATCH* m_colourPickerTopLight;
		wxStaticText* m_staticText6;
		COLOR_SWATCH* m_colourPickerBottomLight;
		wxStaticText* m_staticText20;
		wxStaticText* m_staticText18;
		wxStaticText* m_staticText27;
		wxStaticText* m_staticText28;
		wxStaticText* m_staticText21;
		COLOR_SWATCH* m_colourPickerLight1;
		wxSpinCtrl* m_spinCtrlLightElevation1;
		wxSpinCtrl* m_spinCtrlLightAzimuth1;
		wxStaticText* m_staticText22;
		COLOR_SWATCH* m_colourPickerLight5;
		wxSpinCtrl* m_spinCtrlLightElevation5;
		wxSpinCtrl* m_spinCtrlLightAzimuth5;
		wxStaticText* m_staticText23;
		COLOR_SWATCH* m_colourPickerLight2;
		wxSpinCtrl* m_spinCtrlLightElevation2;
		wxSpinCtrl* m_spinCtrlLightAzimuth2;
		wxStaticText* m_staticText24;
		COLOR_SWATCH* m_colourPickerLight6;
		wxSpinCtrl* m_spinCtrlLightElevation6;
		wxSpinCtrl* m_spinCtrlLightAzimuth6;
		wxStaticText* m_staticText25;
		COLOR_SWATCH* m_colourPickerLight3;
		wxSpinCtrl* m_spinCtrlLightElevation3;
		wxSpinCtrl* m_spinCtrlLightAzimuth3;
		wxStaticText* m_staticText26;
		COLOR_SWATCH* m_colourPickerLight7;
		wxSpinCtrl* m_spinCtrlLightElevation7;
		wxSpinCtrl* m_spinCtrlLightAzimuth7;
		wxStaticText* m_staticText171;
		COLOR_SWATCH* m_colourPickerLight4;
		wxSpinCtrl* m_spinCtrlLightElevation4;
		wxSpinCtrl* m_spinCtrlLightAzimuth4;
		wxStaticText* m_staticText181;
		COLOR_SWATCH* m_colourPickerLight8;
		wxSpinCtrl* m_spinCtrlLightElevation8;
		wxSpinCtrl* m_spinCtrlLightAzimuth8;

	public:

		PANEL_3D_RAYTRACING_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_3D_RAYTRACING_OPTIONS_BASE();

};

