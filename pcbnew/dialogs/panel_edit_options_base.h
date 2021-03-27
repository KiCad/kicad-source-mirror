///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/radiobut.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EDIT_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EDIT_OPTIONS_BASE : public wxPanel
{
	private:

	protected:
		enum
		{
			wxID_SEGMENTS45 = 1000
		};

		wxCheckBox* m_magneticPads;
		wxCheckBox* m_magneticGraphics;
		wxCheckBox* m_flipLeftRight;
		wxCheckBox* m_segments45OnlyCtrl;
		wxStaticText* m_staticTextRotationAngle;
		wxTextCtrl* m_rotationAngle;
		wxStaticBoxSizer* m_mouseCmdsWinLin;
		wxStaticText* m_staticText181;
		wxStaticBoxSizer* m_mouseCmdsOSX;
		wxStaticText* m_staticText1811;
		wxSimplebook* m_optionsBook;
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
		wxCheckBox* m_Show_Page_Limits;
		wxCheckBox* m_Auto_Refill_Zones;
		wxCheckBox* m_Allow_Free_Pads;

	public:

		PANEL_EDIT_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_EDIT_OPTIONS_BASE();

};

