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
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_staticText11;
		wxRadioBox* m_rbUnits;
		wxRadioBox* m_rbZeroFormat;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText6;
		wxStaticText* m_staticText10;
		wxStaticText* m_staticText7;
		wxStaticText* m_staticTextUnitsmm;
		wxChoice* m_choiceIntegerMM;
		wxStaticText* m_staticText8;
		wxChoice* m_choiceMantissaMM;
		wxStaticText* m_staticTextUnitsInch;
		wxChoice* m_choiceIntegerInch;
		wxStaticText* m_staticText9;
		wxChoice* m_choiceMantissaInch;

		// Virtual event handlers, overide them in your derived class
		virtual void onUnitsChange( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 440,336 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_GERBVIEW_EXCELLON_SETTINGS_BASE();

};

