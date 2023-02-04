///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
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
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EDIT_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxBoxSizer* m_sizerBoardEdit;
		wxCheckBox* m_flipLeftRight;
		wxCheckBox* m_allowFreePads;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_cbConstrainHV45Mode;
		wxStaticText* m_rotationAngleLabel;
		wxTextCtrl* m_rotationAngleCtrl;
		wxStaticText* m_rotationAngleUnits;
		wxStaticLine* m_staticline4;
		wxStaticText* m_arcEditModeLabel;
		wxChoice* m_arcEditMode;
		wxStaticBoxSizer* m_mouseCmdsWinLin;
		wxStaticText* m_staticText181;
		wxRadioBox* m_rbCtrlClickAction;
		wxStaticBoxSizer* m_mouseCmdsOSX;
		wxStaticText* m_staticText1811;
		wxRadioBox* m_rbCtrlClickActionMac;
		wxSimplebook* m_optionsBook;
		wxCheckBox* m_magneticPads;
		wxCheckBox* m_magneticGraphics;
		wxStaticText* m_staticText2;
		wxChoice* m_magneticPadChoice;
		wxStaticText* m_staticText21;
		wxChoice* m_magneticTrackChoice;
		wxStaticText* m_staticText211;
		wxChoice* m_magneticGraphicsChoice;
		wxCheckBox* m_showSelectedRatsnest;
		wxCheckBox* m_OptDisplayCurvedRatsnestLines;
		wxStaticText* m_staticText5;
		wxRadioButton* m_rbTrackDragMove;
		wxRadioButton* m_rbTrackDrag45;
		wxRadioButton* m_rbTrackDragFree;
		wxCheckBox* m_escClearsNetHighlight;
		wxCheckBox* m_showPageLimits;
		wxCheckBox* m_cbCourtyardCollisions;
		wxCheckBox* m_autoRefillZones;

	public:

		PANEL_EDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_EDIT_OPTIONS_BASE();

};

