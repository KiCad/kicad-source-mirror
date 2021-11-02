///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-39-g3487c3cb)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include "dialog_shim.h"
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
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
		wxStaticBitmap* m_bitmapTdCircularInfo;
		wxStaticBitmap* m_bitmapTdRectangularInfo;
		wxStaticBitmap* m_bitmapTdTrackInfo;
		wxStaticText* m_stMaxLen;
		wxTextCtrl* m_tcTdMaxLen;
		wxStaticText* m_stLenUnit;
		wxStaticText* m_stHsetting;
		wxSpinCtrlDouble* m_spTeardropLenPercent;
		wxStaticText* m_stLenPercent;
		wxStaticLine* m_staticline5;
		wxStaticLine* m_staticline6;
		wxStaticLine* m_staticline7;
		wxStaticText* m_stTdMaxSize;
		wxTextCtrl* m_tcMaxSize;
		wxStaticText* m_stSizeUnit;
		wxStaticText* m_stVsetting;
		wxSpinCtrlDouble* m_spTeardropSizePercent;
		wxStaticText* m_stTdSizePercent;
		wxCheckBox* m_cbPadVia;
		wxCheckBox* m_cbRoundShapesOnly;
		wxCheckBox* m_cbSmdSimilarPads;
		wxCheckBox* m_cbTrack2Track;
		wxRadioBox* m_rbShape;
		wxStaticText* m_stPoinCount;
		wxSpinCtrl* m_spPointCount;
		wxCheckBox* m_cbOptUseNextTrack;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

	public:

		TEARDROP_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Teardrop Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 477,427 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~TEARDROP_DIALOG_BASE();

};

