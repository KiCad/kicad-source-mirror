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
		wxStaticText* m_stLenPercentLabel;
		wxSpinCtrlDouble* m_spLenPercent;
		wxStaticText* m_stMaxLen;
		wxTextCtrl* m_tcTdMaxLen;
		wxStaticText* m_stMaxLenUnits;
		wxStaticText* m_stWidthLabel;
		wxSpinCtrlDouble* m_spWidthPercent;
		wxStaticText* m_stMaxWidthLabel;
		wxTextCtrl* m_tcMaxWidth;
		wxStaticText* m_stMaxWidthUnits;
		wxCheckBox* m_cbPreferZoneConnection;
		wxStaticText* m_stHDRatio;
		wxSpinCtrlDouble* m_spTeardropHDPercent;
		wxCheckBox* m_cbTeardropsUseNextTrack;
		wxCheckBox* m_cbCurvedEdges;
		wxStaticText* m_rectShapesLabel;
		wxStaticLine* m_staticline2;
		wxStaticBitmap* m_bitmapTeardrop1;
		wxStaticText* m_stLenPercent1Label;
		wxSpinCtrlDouble* m_spLenPercent1;
		wxStaticText* m_stMaxLen1;
		wxTextCtrl* m_tcTdMaxLen1;
		wxStaticText* m_stMaxLen1Units;
		wxStaticText* m_stWidth1Label;
		wxSpinCtrlDouble* m_spWidthPercent1;
		wxStaticText* m_stMaxWidth1Label;
		wxTextCtrl* m_tcMaxWidth1;
		wxStaticText* m_stMaxWidth1Units;
		wxCheckBox* m_cbPreferZoneConnection1;
		wxStaticText* m_stHDRatio1;
		wxSpinCtrlDouble* m_spTeardropHDPercent1;
		wxCheckBox* m_cbTeardropsUseNextTrack1;
		wxCheckBox* m_cbCurvedEdges1;
		wxStaticText* m_tracksLabel;
		wxStaticLine* m_staticline3;
		wxStaticBitmap* m_bitmapTeardrop2;
		wxStaticText* m_stLenPercent2Label;
		wxSpinCtrlDouble* m_spLenPercent2;
		wxStaticText* m_stMaxLen2;
		wxTextCtrl* m_tcTdMaxLen2;
		wxStaticText* m_stMaxLen2Units;
		wxStaticText* m_stWidth2Label;
		wxSpinCtrlDouble* m_spWidthPercent2;
		wxStaticText* m_stMaxWidth2Label;
		wxTextCtrl* m_tcMaxWidth2;
		wxStaticText* m_stMaxWidth2Units;
		wxStaticText* m_stHDRatio2;
		wxSpinCtrlDouble* m_spTeardropHDPercent2;
		wxCheckBox* m_cbTeardropsUseNextTrack2;
		wxCheckBox* m_cbCurvedEdges2;

	public:

		PANEL_SETUP_TEARDROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_TEARDROPS_BASE();

};

