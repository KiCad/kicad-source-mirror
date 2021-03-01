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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_MOUSE_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_MOUSE_SETTINGS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxCheckBox* m_checkZoomCenter;
		wxCheckBox* m_checkAutoPan;
		wxCheckBox* m_checkZoomAcceleration;
		wxBoxSizer* m_zoomSizer;
		wxStaticText* m_staticText1;
		wxSlider* m_zoomSpeed;
		wxCheckBox* m_checkAutoZoomSpeed;
		wxBoxSizer* m_panSizer;
		wxStaticText* m_staticText22;
		wxSlider* m_autoPanSpeed;
		wxStaticText* m_leftButtonDragLabel;
		wxChoice* m_choiceLeftButtonDrag;
		wxStaticText* m_staticText3;
		wxChoice* m_choiceMiddleButtonDrag;
		wxStaticText* m_staticText31;
		wxChoice* m_choiceRightButtonDrag;
		wxStaticText* m_staticText21;
		wxStaticBitmap* m_scrollWarning;
		wxStaticText* m_staticText19;
		wxStaticText* m_staticText17;
		wxStaticText* m_lblCtrl;
		wxStaticText* m_staticText8;
		wxStaticText* m_lblAlt;
		wxStaticText* m_staticText10;
		wxRadioButton* m_rbZoomNone;
		wxRadioButton* m_rbZoomCtrl;
		wxRadioButton* m_rbZoomShift;
		wxRadioButton* m_rbZoomAlt;
		wxStaticText* m_staticText11;
		wxRadioButton* m_rbPanVNone;
		wxRadioButton* m_rbPanVCtrl;
		wxRadioButton* m_rbPanVShift;
		wxRadioButton* m_rbPanVAlt;
		wxStaticText* m_staticText20;
		wxRadioButton* m_rbPanHNone;
		wxRadioButton* m_rbPanHCtrl;
		wxRadioButton* m_rbPanHShift;
		wxRadioButton* m_rbPanHAlt;
		wxCheckBox* m_checkEnablePanH;
		wxButton* m_mouseDefaults;
		wxButton* m_trackpadDefaults;

		// Virtual event handlers, overide them in your derived class
		virtual void OnScrollRadioButton( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMouseDefaults( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTrackpadDefaults( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_MOUSE_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_MOUSE_SETTINGS_BASE();

};

