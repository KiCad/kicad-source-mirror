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
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/radiobut.h>
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
		wxScrolledWindow* m_scrolledWindow1;
		wxStaticText* m_staticText26;
		wxStaticBitmap* m_bitmapBlindBuried;
		wxCheckBox* m_OptAllowBlindBuriedVias;
		wxStaticBitmap* m_bitmap_uVia;
		wxCheckBox* m_OptAllowMicroVias;
		wxStaticLine* m_staticline2;
		wxStaticText* m_stCircleToPolyOpt;
		wxStaticText* m_maxErrorTitle;
		wxTextCtrl* m_maxErrorCtrl;
		wxStaticText* m_maxErrorUnits;
		wxStaticText* m_stCircleToPolyWarning;
		wxBoxSizer* m_bSizerPolygonFillOption;
		wxStaticLine* m_staticline1;
		wxStaticText* m_stZoneFilledPolysOpt;
		wxStaticBitmap* m_bitmapZoneFillOpt;
		wxRadioButton* m_rbOutlinePolygonBestQ;
		wxRadioButton* m_rbOutlinePolygonFastest;
		wxStaticBitmap* m_filletBitmap;
		wxCheckBox* m_allowExternalFilletsOpt;
		wxStaticLine* m_staticline15;
		wxStaticText* m_staticText33;
		wxCheckBox* m_useHeightForLengthCalcs;
		wxStaticText* m_staticText23;
		wxStaticBitmap* m_bitmapClearance;
		wxStaticText* m_clearanceTitle;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticBitmap* m_bitmapMinTrackWidth;
		wxStaticText* m_TrackMinWidthTitle;
		wxTextCtrl* m_TrackMinWidthCtrl;
		wxStaticText* m_TrackMinWidthUnits;
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
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticLine* m_staticline6;
		wxStaticText* m_staticText24;
		wxStaticBitmap* m_bitmapMinViaDrill;
		wxStaticText* m_MinDrillTitle;
		wxTextCtrl* m_MinDrillCtrl;
		wxStaticText* m_MinDrillUnits;
		wxStaticBitmap* m_bitmapMinHoleClearance;
		wxStaticText* m_HoleToHoleTitle;
		wxTextCtrl* m_SetHoleToHoleCtrl;
		wxStaticText* m_HoleToHoleUnits;
		wxStaticLine* m_staticline8;
		wxStaticLine* m_staticline9;
		wxStaticLine* m_staticline10;
		wxStaticLine* m_staticline11;
		wxStaticText* m_staticText25;
		wxStaticBitmap* m_bitmapMinuViaDiameter;
		wxStaticText* m_uviaMinSizeLabel;
		wxTextCtrl* m_uviaMinSizeCtrl;
		wxStaticText* m_uviaMinSizeUnits;
		wxStaticBitmap* m_bitmapMinuViaDrill;
		wxStaticText* m_uviaMinDrillLabel;
		wxTextCtrl* m_uviaMinDrillCtrl;
		wxStaticText* m_uviaMinDrillUnits;
		wxStaticLine* m_staticline111;
		wxStaticLine* m_staticline12;
		wxStaticLine* m_staticline13;
		wxStaticLine* m_staticline14;
		wxStaticText* m_staticText28;
		wxStaticText* m_silkClearanceLabel;
		wxTextCtrl* m_silkClearanceCtrl;
		wxStaticText* m_silkClearanceUnits;

		// Virtual event handlers, overide them in your derived class
		virtual void onChangeOutlineOpt( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_CONSTRAINTS_BASE();

};

