///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
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
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class TEARDROP_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class TEARDROP_DIALOG_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextRndS;
		wxStaticBitmap* m_bitmapTdCircularInfo;
		wxStaticText* m_stHsettingRound;
		wxSpinCtrlDouble* m_spTeardropLenPercentRound;
		wxStaticText* m_stTeardropLenUnits;
		wxStaticText* m_stVsettingRound;
		wxSpinCtrlDouble* m_spTeardropSizePercentRound;
		wxStaticText* m_stLenPercentRound;
		wxStaticText* m_stMaxLenRound;
		wxTextCtrl* m_tcTdMaxLenRound;
		wxStaticText* m_stMaxLenRoundUnits;
		wxStaticText* m_stTdMaxSizeRound;
		wxTextCtrl* m_tcMaxHeightRound;
		wxStaticText* m_stMaxHeightRoundUnits;
		wxStaticText* m_stHDRatioRound;
		wxSpinCtrlDouble* m_spTeardropHDPercentRound;
		wxStaticText* m_stHDPercentRound;
		wxRadioBox* m_rbShapeRound;
		wxStaticLine* m_staticline2;
		wxStaticLine* m_staticline3;
		wxStaticLine* m_staticline4;
		wxStaticLine* m_staticline5;
		wxStaticText* m_staticTextRectS;
		wxStaticBitmap* m_bitmapTdRectangularInfo;
		wxStaticText* m_stHsettingRect;
		wxSpinCtrlDouble* m_spTeardropLenPercentRect;
		wxStaticText* m_stHDPercentRect;
		wxStaticText* m_stVsettingRect;
		wxSpinCtrlDouble* m_spTeardropSizePercentRect;
		wxStaticText* m_stLenPercentRect;
		wxStaticText* m_stMaxLenRect;
		wxTextCtrl* m_tcTdMaxLenRect;
		wxStaticText* m_stMaxLenRectUnits;
		wxStaticText* m_stTdMaxSizeRect;
		wxTextCtrl* m_tcMaxHeightRect;
		wxStaticText* m_stMaxHeightRectUnits;
		wxStaticText* m_stHDRatioRect;
		wxSpinCtrlDouble* m_spTeardropHDPercentRect;
		wxStaticText* m_stTeardropHUnits;
		wxRadioBox* m_rbShapeRect;
		wxStaticLine* m_staticline6;
		wxStaticLine* m_staticline7;
		wxStaticLine* m_staticline8;
		wxStaticLine* m_staticline9;
		wxStaticText* m_staticTextTrck;
		wxStaticBitmap* m_bitmapTdTrackInfo;
		wxStaticText* m_stHsettingtrack;
		wxSpinCtrlDouble* m_spTeardropLenPercentTrack;
		wxStaticText* m_stTeardropLenTrackUnits;
		wxStaticText* m_stVsettingtrack;
		wxSpinCtrlDouble* m_spTeardropSizePercentTrack;
		wxStaticText* m_stLenPercentTrack;
		wxStaticText* m_stMaxLenTrack;
		wxTextCtrl* m_tcTdMaxLenTrack;
		wxStaticText* m_stMaxLenTrackUnits;
		wxStaticText* m_stTdMaxSizeTrack;
		wxTextCtrl* m_tcMaxHeightTrack;
		wxStaticText* m_stMaxHeightTrackUnits;
		wxStaticText* m_stHDRatioTrack;
		wxSpinCtrlDouble* m_spTeardropHDPercentTrack;
		wxStaticText* m_stHDPercentTrack;
		wxRadioBox* m_rbShapeTrack;
		wxCheckBox* m_cbPadVia;
		wxCheckBox* m_cbRoundShapesOnly;
		wxCheckBox* m_cbSmdSimilarPads;
		wxCheckBox* m_cbTrack2Track;
		wxCheckBox* m_cbOptUseNextTrack;
		wxCheckBox* m_cbPadsInZones;
		wxStaticText* m_stPointCount;
		wxSpinCtrl* m_spPointCount;
		wxCheckBox* m_generateRawTeardrops;
		wxStaticText* m_rawTeardropsHint;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		TEARDROP_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Add Teardrops"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~TEARDROP_DIALOG_BASE();

};

