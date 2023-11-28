///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_TEARDROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_TEARDROPS_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_gridSizer;
		wxStaticText* m_roundShapesLabel;
		wxStaticLine* m_staticline1;
		wxStaticBitmap* m_bitmapTeardrop;
		wxStaticText* m_edgesLabel;
		wxRadioButton* m_rbStraightLines;
		wxRadioButton* m_rbCurved;
		wxStaticText* m_stLenPercentLabel;
		wxSpinCtrlDouble* m_spLenPercent;
		wxStaticText* m_stLenPercentUnits;
		wxStaticText* m_staticText76;
		wxStaticText* m_stMaxLen;
		wxTextCtrl* m_tcTdMaxLen;
		wxStaticText* m_stMaxLenUnits;
		wxStaticText* m_stWidthLabel;
		wxSpinCtrlDouble* m_spWidthPercent;
		wxStaticText* m_stWidthPercentUnits;
		wxStaticText* m_staticText77;
		wxStaticText* m_stMaxWidthLabel;
		wxTextCtrl* m_tcMaxWidth;
		wxStaticText* m_stMaxWidthUnits;
		wxStaticText* m_curvePointsLabel;
		wxSpinCtrl* m_curvePointsCtrl;
		wxCheckBox* m_cbPreferZoneConnection;
		wxStaticText* m_stHDRatio;
		wxSpinCtrlDouble* m_spTeardropHDPercent;
		wxStaticText* m_minTrackWidthUnits;
		wxStaticText* m_staticText78;
		wxCheckBox* m_cbTeardropsUseNextTrack;
		wxStaticText* m_rectShapesLabel;
		wxStaticLine* m_staticline2;
		wxStaticBitmap* m_bitmapTeardrop1;
		wxStaticText* m_edgesLabel1;
		wxRadioButton* m_rbStraightLines1;
		wxRadioButton* m_rbCurved1;
		wxStaticText* m_stLenPercentLabel1;
		wxSpinCtrlDouble* m_spLenPercent1;
		wxStaticText* m_stLenPercentUnits1;
		wxStaticText* m_staticText75;
		wxStaticText* m_stMaxLen1;
		wxTextCtrl* m_tcTdMaxLen1;
		wxStaticText* m_stMaxLenUnits1;
		wxStaticText* m_stWidthLabel1;
		wxSpinCtrlDouble* m_spWidthPercent1;
		wxStaticText* m_stWidthPercentUnits1;
		wxStaticText* m_staticText74;
		wxStaticText* m_stMaxWidthLabel1;
		wxTextCtrl* m_tcMaxWidth1;
		wxStaticText* m_stMaxWidthUnits1;
		wxStaticText* m_curvePointsLabel1;
		wxSpinCtrl* m_curvePointsCtrl1;
		wxCheckBox* m_cbPreferZoneConnection1;
		wxStaticText* m_stHDRatio1;
		wxSpinCtrlDouble* m_spTeardropHDPercent1;
		wxStaticText* m_minTrackWidthUnits1;
		wxStaticText* m_staticText73;
		wxCheckBox* m_cbTeardropsUseNextTrack1;
		wxStaticText* m_tracksLabel;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmapTeardrop2;
		wxStaticText* m_edgesLabel2;
		wxRadioButton* m_rbStraightLines2;
		wxRadioButton* m_rbCurved2;
		wxStaticText* m_stLenPercentLabel2;
		wxSpinCtrlDouble* m_spLenPercent2;
		wxStaticText* m_stLenPercentUnits2;
		wxStaticText* m_staticText70;
		wxStaticText* m_stMaxLen2;
		wxTextCtrl* m_tcTdMaxLen2;
		wxStaticText* m_stMaxLenUnits2;
		wxStaticText* m_stWidthLabel2;
		wxSpinCtrlDouble* m_spWidthPercent2;
		wxStaticText* m_stWidthPercentUnits2;
		wxStaticText* m_staticText72;
		wxStaticText* m_stMaxWidthLabel2;
		wxTextCtrl* m_tcMaxWidth2;
		wxStaticText* m_stMaxWidthUnits2;
		wxStaticText* m_curvePointsLabel2;
		wxSpinCtrl* m_curvePointsCtrl2;
		wxStaticText* m_stHDRatio2;
		wxSpinCtrlDouble* m_spTeardropHDPercent2;
		wxStaticText* m_minTrackWidthUnits2;
		wxStaticText* m_staticText71;
		wxCheckBox* m_cbTeardropsUseNextTrack2;

	public:

		PANEL_SETUP_TEARDROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_TEARDROPS_BASE();

};

