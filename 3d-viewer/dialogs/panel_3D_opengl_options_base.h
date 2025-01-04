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
class COLOR_SWATCH;

#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_3D_OPENGL_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_3D_OPENGL_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_renderingLabel;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_checkBoxBoundingBoxes;
		wxCheckBox* m_checkBoxCuThickness;
		wxCheckBox* m_checkBoxHighlightOnRollOver;
		wxStaticText* m_staticText221;
		wxChoice* m_choiceAntiAliasing;
		wxStaticText* m_selectionColorLabel;
		COLOR_SWATCH* m_selectionColorSwatch;
		wxStaticText* m_movingLabel;
		wxStaticLine* m_staticline2;
		wxCheckBox* m_checkBoxDisableAAMove;
		wxCheckBox* m_checkBoxDisableMoveThickness;
		wxCheckBox* m_checkBoxDisableMoveVias;
		wxCheckBox* m_checkBoxDisableMoveHoles;

	public:

		PANEL_3D_OPENGL_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_3D_OPENGL_OPTIONS_BASE();

};

