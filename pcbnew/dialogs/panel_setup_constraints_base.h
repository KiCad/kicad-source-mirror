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
#include <wx/valtext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_CONSTRAINTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_CONSTRAINTS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText23;
		wxStaticLine* m_staticline151;
		wxStaticLine* m_staticline16;
		wxStaticLine* m_staticline17;
		wxStaticLine* m_staticline18;
		wxStaticBitmap* m_bitmapClearance;
		wxStaticText* m_clearanceTitle;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticBitmap* m_bitmapMinTrackWidth;
		wxStaticText* m_TrackMinWidthTitle;
		wxTextCtrl* m_TrackMinWidthCtrl;
		wxStaticText* m_TrackMinWidthUnits;
		wxStaticBitmap* m_bitmapMinConn;
		wxStaticText* m_MinConnTitle;
		wxTextCtrl* m_MinConnCtrl;
		wxStaticText* m_MinConnUnits;
		wxStaticBitmap* m_bitmapMinViaAnnulus;
		wxStaticText* m_ViaMinAnnulusTitle;
		wxTextCtrl* m_ViaMinAnnulusCtrl;
		wxStaticText* m_ViaMinAnnulusUnits;
		wxStaticBitmap* m_bitmapMinViaDiameter;
		wxStaticText* m_ViaMinTitle;
		wxTextCtrl* m_SetViasMinSizeCtrl;
		wxStaticText* m_ViaMinUnits;
		wxStaticBitmap* m_bitmapHoleClearance;
		wxStaticText* m_HoleClearanceLabel;
		wxTextCtrl* m_HoleClearanceCtrl;
		wxStaticText* m_HoleClearanceUnits;
		wxStaticBitmap* m_bitmapEdgeClearance;
		wxStaticText* m_EdgeClearanceLabel;
		wxTextCtrl* m_EdgeClearanceCtrl;
		wxStaticText* m_EdgeClearanceUnits;
		wxStaticBitmap* m_bitmapMinGrooveWidth;
		wxStaticText* m_minGrooveWidthLabel;
		wxTextCtrl* m_minGrooveWidthCtrl;
		wxStaticText* m_minGrooveWidthUnits;
		wxStaticText* m_staticText24;
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticLine* m_staticline6;
		wxStaticBitmap* m_bitmapMinViaDrill;
		wxStaticText* m_MinDrillTitle;
		wxTextCtrl* m_MinDrillCtrl;
		wxStaticText* m_MinDrillUnits;
		wxStaticBitmap* m_bitmapMinHoleClearance;
		wxStaticText* m_HoleToHoleTitle;
		wxTextCtrl* m_SetHoleToHoleCtrl;
		wxStaticText* m_HoleToHoleUnits;
		wxStaticText* m_staticText25;
		wxStaticLine* m_staticline8;
		wxStaticLine* m_staticline9;
		wxStaticLine* m_staticline10;
		wxStaticLine* m_staticline11;
		wxStaticBitmap* m_bitmapMinuViaDiameter;
		wxStaticText* m_uviaMinSizeLabel;
		wxTextCtrl* m_uviaMinSizeCtrl;
		wxStaticText* m_uviaMinSizeUnits;
		wxStaticBitmap* m_bitmapMinuViaDrill;
		wxStaticText* m_uviaMinDrillLabel;
		wxTextCtrl* m_uviaMinDrillCtrl;
		wxStaticText* m_uviaMinDrillUnits;
		wxStaticText* m_staticText28;
		wxStaticLine* m_staticline111;
		wxStaticLine* m_staticline12;
		wxStaticLine* m_staticline13;
		wxStaticLine* m_staticline14;
		wxStaticText* m_silkClearanceLabel;
		wxTextCtrl* m_silkClearanceCtrl;
		wxStaticText* m_silkClearanceUnits;
		wxStaticText* m_textHeightLabel;
		wxTextCtrl* m_textHeightCtrl;
		wxStaticText* m_textHeightUnits;
		wxStaticText* m_textThicknessLabel;
		wxTextCtrl* m_textThicknessCtrl;
		wxStaticText* m_textThicknessUnits;
		wxStaticText* m_stCircleToPolyOpt;
		wxStaticLine* m_staticline19;
		wxStaticText* m_maxErrorTitle;
		wxTextCtrl* m_maxErrorCtrl;
		wxStaticText* m_maxErrorUnits;
		wxStaticText* m_stCircleToPolyWarning;
		wxBoxSizer* m_bSizerPolygonFillOption;
		wxStaticText* m_stZoneFilledPolysOpt;
		wxStaticLine* m_staticline1;
		wxStaticBitmap* m_filletBitmap;
		wxCheckBox* m_allowExternalFilletsOpt;
		wxStaticBitmap* m_spokeBitmap;
		wxStaticText* m_minResolvedSpokesLabel;
		wxSpinCtrl* m_minResolvedSpokeCountCtrl;
		wxStaticText* m_staticText33;
		wxStaticLine* m_staticline15;
		wxCheckBox* m_useHeightForLengthCalcs;

	public:
		wxScrolledWindow* m_scrolledWindow;

		PANEL_SETUP_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_CONSTRAINTS_BASE();

};

