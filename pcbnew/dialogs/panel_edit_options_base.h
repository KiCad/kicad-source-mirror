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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EDIT_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:
		wxStaticText* stMagneticPtsLabel1;
		wxStaticText* stRatsnestLabel;
		wxStaticText* stMiscellaneousLabel;

	protected:
		wxStaticText* m_staticText31;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_cbConstrainHV45Mode;
		wxStaticText* m_rotationAngleLabel;
		wxTextCtrl* m_rotationAngleCtrl;
		wxStaticText* m_rotationAngleUnits;
		wxStaticText* m_arcEditModeLabel;
		wxChoice* m_arcEditMode;
		wxBoxSizer* m_sizerBoardEdit;
		wxStaticText* m_trackMouseDragLabel;
		wxChoice* m_trackMouseDragCtrl;
		wxStaticText* m_staticText33;
		wxRadioButton* m_rbFlipLeftRight;
		wxRadioButton* m_rbFlipTopBottom;
		wxCheckBox* m_allowFreePads;
		wxStaticText* m_staticText32;
		wxStaticLine* m_staticline4;
		wxBoxSizer* m_mouseCmdsWinLin;
		wxStaticText* m_stHint1;
		wxRadioButton* m_rbToggleSel;
		wxRadioButton* m_rbHighlightNet;
		wxBoxSizer* m_mouseCmdsOSX;
		wxStaticText* m_stHint2;
		wxRadioButton* m_rbToggleSelMac;
		wxRadioButton* m_rbHighlightNetMac;
		wxSimplebook* m_optionsBook;
		wxStaticText* m_staticText34;
		wxStaticLine* m_staticline5;
		wxCheckBox* m_magneticPads;
		wxCheckBox* m_magneticGraphics;
		wxStaticLine* m_staticline6;
		wxStaticText* m_staticText2;
		wxChoice* m_magneticPadChoice;
		wxStaticText* m_staticText21;
		wxChoice* m_magneticTrackChoice;
		wxStaticText* m_staticText211;
		wxChoice* m_magneticGraphicsChoice;
		wxStaticLine* m_staticline7;
		wxCheckBox* m_showSelectedRatsnest;
		wxCheckBox* m_OptDisplayCurvedRatsnestLines;
		wxStaticText* m_ratsnestThicknessLabel;
		wxSpinCtrlDouble* m_ratsnestThickness;
		wxStaticLine* m_staticline8;
		wxCheckBox* m_escClearsNetHighlight;
		wxCheckBox* m_showPageLimits;
		wxCheckBox* m_cbCourtyardCollisions;
		wxCheckBox* m_autoRefillZones;

	public:

		PANEL_EDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_EDIT_OPTIONS_BASE();

};

