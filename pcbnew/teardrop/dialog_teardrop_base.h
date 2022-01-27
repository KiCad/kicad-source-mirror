///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
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
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/statline.h>
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
		wxStaticText* m_stMaxLenRound;
		wxTextCtrl* m_tcTdMaxLenRound;
		wxStaticText* m_stTdMaxSizeRound;
		wxTextCtrl* m_tcMaxHeightRound;
		wxStaticText* m_stLenUnitRound;
		wxStaticText* m_stHsettingRound;
		wxSpinCtrlDouble* m_spTeardropLenPercentRound;
		wxStaticText* m_stVsettingRound;
		wxSpinCtrlDouble* m_spTeardropSizePercentRound;
		wxStaticText* m_stLenPercentRound;
		wxStaticText* m_staticTextRectS;
		wxStaticBitmap* m_bitmapTdRectangularInfo;
		wxStaticText* m_stMaxLenRect;
		wxTextCtrl* m_tcTdMaxLenRect;
		wxStaticText* m_stTdMaxSizeRect;
		wxTextCtrl* m_tcMaxHeightRect;
		wxStaticText* m_stLenUnitRect;
		wxStaticText* m_stHsettingRect;
		wxSpinCtrlDouble* m_spTeardropLenPercentRect;
		wxStaticText* m_stVsettingRect;
		wxSpinCtrlDouble* m_spTeardropSizePercentRect;
		wxStaticText* m_stLenPercentRect;
		wxStaticText* m_staticTextTrck;
		wxStaticBitmap* m_bitmapTdTrackInfo;
		wxStaticText* m_stMaxLenTrack;
		wxTextCtrl* m_tcTdMaxLenTrack;
		wxStaticText* m_stTdMaxSizeTrack;
		wxTextCtrl* m_tcMaxHeightTrack;
		wxStaticText* m_stLenUnitTrack;
		wxStaticText* m_stHsettingtrack;
		wxSpinCtrlDouble* m_spTeardropLenPercentTrack;
		wxStaticText* m_stVsettingtrack;
		wxSpinCtrlDouble* m_spTeardropSizePercentTrack;
		wxStaticText* m_stLenPercentTrack;
		wxCheckBox* m_cbPadVia;
		wxCheckBox* m_cbRoundShapesOnly;
		wxCheckBox* m_cbSmdSimilarPads;
		wxCheckBox* m_cbTrack2Track;
		wxRadioBox* m_rbShapeRound;
		wxRadioBox* m_rbShapeRect;
		wxRadioBox* m_rbShapeTrack;
		wxCheckBox* m_cbOptUseNextTrack;
		wxCheckBox* m_cbPadsInZones;
		wxStaticText* m_stPointCount;
		wxSpinCtrl* m_spPointCount;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		TEARDROP_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Teardrop Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 648,479 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~TEARDROP_DIALOG_BASE();

};

