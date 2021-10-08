///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COLOR_CODE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COLOR_CODE_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxRadioBox* m_rbToleranceSelection;
		wxStaticText* m_staticTextBand1;
		wxStaticText* m_staticTextBand2;
		wxStaticText* m_staticTextBand3;
		wxStaticText* m_staticTextBand4;
		wxStaticText* m_staticTextBand5;
		wxStaticText* m_staticTextBand6;
		wxStaticBitmap* m_Band1bitmap;
		wxStaticBitmap* m_Band2bitmap;
		wxStaticBitmap* m_Band3bitmap;
		wxStaticBitmap* m_Band4bitmap;
		wxStaticBitmap* m_Band_mult_bitmap;
		wxStaticBitmap* m_Band_tol_bitmap;

		// Virtual event handlers, override them in your derived class
		virtual void OnToleranceSelection( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COLOR_CODE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_COLOR_CODE_BASE();

};

