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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SIMULATOR_PREFERENCES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SIMULATOR_PREFERENCES_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_lblScrollHeading;
		wxStaticLine* m_scrollLine;
		wxStaticText* m_lblVScrollMovement;
		wxStaticText* m_lblVScrollModifier;
		wxStaticText* m_lblVScrollAction;
		wxStaticText* m_lblVScrollUnmodified;
		wxChoice* m_choiceVScrollUnmodified;
		wxStaticText* m_lblVScrollCtrl;
		wxChoice* m_choiceVScrollCtrl;
		wxStaticText* m_lblVScrollShift;
		wxChoice* m_choiceVScrollShift;
		wxStaticText* m_lblVScrollAlt;
		wxChoice* m_choiceVScrollAlt;
		wxStaticText* m_lblHScrollMovement;
		wxStaticText* m_lblHScrollModifier;
		wxStaticText* m_lblHScrollAction;
		wxStaticText* m_lblHScrollAny;
		wxChoice* m_choiceHScroll;
		wxButton* m_btnMouseDefaults;
		wxButton* m_btnTrackpadDefaults;

		// Virtual event handlers, override them in your derived class
		virtual void onMouseDefaults( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTrackpadDefaults( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SIMULATOR_PREFERENCES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SIMULATOR_PREFERENCES_BASE();

};

