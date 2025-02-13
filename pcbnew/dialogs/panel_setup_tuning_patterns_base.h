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
class TEXT_CTRL_EVAL;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_TUNING_PATTERNS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_TUNING_PATTERNS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_singleTrackLabel;
		wxStaticLine* m_staticline1;
		wxStaticBitmap* m_singleTrackLegend;
		wxStaticText* m_track_minALabel;
		wxTextCtrl* m_track_minACtrl;
		wxStaticText* m_track_minAUnits;
		wxStaticText* m_track_maxALabel;
		wxTextCtrl* m_track_maxACtrl;
		wxStaticText* m_track_maxAUnits;
		wxStaticText* m_track_spacingLabel;
		wxTextCtrl* m_track_spacingCtrl;
		wxStaticText* m_track_spacingUnits;
		wxStaticText* m_track_cornerLabel;
		wxChoice* m_track_cornerCtrl;
		wxStaticText* m_track_rLabel;
		TEXT_CTRL_EVAL* m_track_rCtrl;
		wxStaticText* m_track_rUnits;
		wxCheckBox* m_track_singleSided;
		wxStaticText* m_diffPairsLabel;
		wxStaticLine* m_staticline11;
		wxStaticBitmap* m_diffPairLegend;
		wxStaticText* m_dp_minALabel;
		wxTextCtrl* m_dp_minACtrl;
		wxStaticText* m_dp_minAUnits;
		wxStaticText* m_dp_maxALabel;
		wxTextCtrl* m_dp_maxACtrl;
		wxStaticText* m_dp_maxAUnits;
		wxStaticText* m_dp_spacingLabel;
		wxTextCtrl* m_dp_spacingCtrl;
		wxStaticText* m_dp_spacingUnits;
		wxStaticText* m_dp_cornerLabel;
		wxChoice* m_dp_cornerCtrl;
		wxStaticText* m_dp_rLabel;
		TEXT_CTRL_EVAL* m_dp_rCtrl;
		wxStaticText* m_dp_rUnits;
		wxCheckBox* m_dp_singleSided;
		wxStaticText* m_diffPairsLabel1;
		wxStaticLine* m_staticline111;
		wxStaticBitmap* m_skewLegend;
		wxStaticText* m_skew_minALabel;
		wxTextCtrl* m_skew_minACtrl;
		wxStaticText* m_skew_minAUnits;
		wxStaticText* m_skew_maxALabel;
		wxTextCtrl* m_skew_maxACtrl;
		wxStaticText* m_skew_maxAUnits;
		wxStaticText* m_skew_spacingLabel;
		wxTextCtrl* m_skew_spacingCtrl;
		wxStaticText* m_skew_spacingUnits;
		wxStaticText* m_skew_cornerLabel;
		wxChoice* m_skew_cornerCtrl;
		wxStaticText* m_skew_rLabel;
		TEXT_CTRL_EVAL* m_skew_rCtrl;
		wxStaticText* m_skew_rUnits;
		wxCheckBox* m_skew_singleSided;

	public:

		PANEL_SETUP_TUNING_PATTERNS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_TUNING_PATTERNS_BASE();

};

