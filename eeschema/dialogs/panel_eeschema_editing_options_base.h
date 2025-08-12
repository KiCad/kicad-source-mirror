///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/simplebook.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EESCHEMA_EDITING_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EESCHEMA_EDITING_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_editingLabel;
		wxStaticLine* m_staticline3;
		wxStaticText* m_staticText24;
		wxChoice* m_choiceLineMode;
		wxStaticText* m_staticTextArcEdit;
		wxChoice* m_choiceArcMode;
		wxCheckBox* m_mouseDragIsDrag;
		wxCheckBox* m_cbAutoStartWires;
		wxCheckBox* m_escClearsNetHighlight;
		wxCheckBox* m_checkAutoAnnotate;
		wxStaticText* m_staticText26;
		wxStaticLine* m_staticline4;
		wxStaticText* m_borderColorLabel;
		COLOR_SWATCH* m_borderColorSwatch;
		wxStaticText* m_backgroundColorLabel;
		COLOR_SWATCH* m_backgroundColorSwatch;
		wxStaticText* m_powerSymbolLabel;
		wxChoice* m_choicePower;
		wxSimplebook* m_leftClickCmdsBook;
		wxPanel* m_pageWinLin;
		wxStaticText* m_leftClickLabel;
		wxStaticLine* m_staticline6;
		wxStaticText* m_hint1;
		wxStaticText* m_staticText91;
		wxStaticText* m_staticText101;
		wxStaticText* m_staticText131;
		wxStaticText* m_staticText141;
		wxStaticText* m_staticText151;
		wxStaticText* m_staticText161;
		wxPanel* m_pageMac;
		wxStaticText* m_leftClickLabel1;
		wxStaticLine* m_staticline7;
		wxStaticText* m_hint2;
		wxStaticText* m_staticText11;
		wxStaticText* m_staticText12;
		wxStaticText* m_staticText9;
		wxStaticText* m_staticText10;
		wxStaticText* m_staticText15;
		wxStaticText* m_staticText16;
		wxStaticText* m_staticText13;
		wxStaticText* m_staticText14;
		wxStaticText* m_staticText32;
		wxStaticLine* m_staticline10;
		wxCheckBox* m_checkAutoplaceFields;
		wxCheckBox* m_checkAutoplaceJustify;
		wxCheckBox* m_checkAutoplaceAlign;
		wxStaticText* m_staticText321;
		wxStaticLine* m_staticline9;
		wxStaticText* m_hPitchLabel;
		wxTextCtrl* m_hPitchCtrl;
		wxStaticText* m_hPitchUnits;
		wxStaticText* m_vPitchLabel;
		wxTextCtrl* m_vPitchCtrl;
		wxStaticText* m_vPitchUnits;
		wxStaticText* m_labelIncrementLabel;
		wxSpinCtrl* m_spinLabelRepeatStep;
		wxStaticText* m_staticText322;
		wxStaticLine* m_staticline8;
		wxCheckBox* m_footprintPreview;
		wxCheckBox* m_neverShowRescue;

	public:

		PANEL_EESCHEMA_EDITING_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_EESCHEMA_EDITING_OPTIONS_BASE();

};

