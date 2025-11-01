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
class HTML_WINDOW;

#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/html/htmlwin.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ELECTRICAL_SPACING_IEC60664_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ELECTRICAL_SPACING_IEC60664_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxScrolledWindow* m_scrolledWindow;
		wxStaticText* m_stTitle;
		wxStaticText* m_staticText5211;
		wxTextCtrl* m_ratedVoltage;
		wxStaticText* m_staticText52112;
		wxStaticText* m_staticText52111;
		wxChoice* m_OVCchoice;
		wxStaticText* m_staticText111111;
		wxTextCtrl* m_impulseVotlage1TxtCtrl;
		wxStaticText* static_textkV;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_RMSVoltage;
		wxStaticText* m_staticText11212;
		wxStaticText* m_staticText114;
		wxTextCtrl* m_transientOvervoltage;
		wxStaticText* m_staticText1121;
		wxStaticText* m_staticText113;
		wxTextCtrl* m_peakVoltage;
		wxStaticText* m_staticText11211;
		wxStaticText* m_staticText112;
		wxChoice* m_insulationType;
		wxStaticText* m_staticText52;
		wxChoice* m_pollutionDegree;
		wxStaticText* m_materialGroupTxt;
		wxChoice* m_materialGroup;
		wxStaticText* m_staticText1112;
		wxCheckBox* m_pcbMaterial;
		wxStaticText* m_staticText1112121;
		wxTextCtrl* m_altitude;
		wxStaticText* m_staticText11121211;
		wxStaticText* m_staticText11111;
		wxTextCtrl* m_clearance;
		wxStaticText* m_staticText71111;
		wxStaticText* m_staticText1111;
		wxTextCtrl* m_creepage;
		wxStaticText* m_staticText7111;
		wxStaticText* m_staticText111;
		wxTextCtrl* m_minGrooveWidth;
		wxStaticText* m_staticText711;
		wxStaticBitmap* m_creepageclearanceBitmap;
		wxStaticText* m_stBitmapLegend;
		HTML_WINDOW* m_panelHelp;

		// Virtual event handlers, override them in your derived class
		virtual void UpdateTransientImpulse( wxCommandEvent& event ) { event.Skip(); }
		virtual void UpdateClearanceCreepage( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_ELECTRICAL_SPACING_IEC60664_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_ELECTRICAL_SPACING_IEC60664_BASE();

};

