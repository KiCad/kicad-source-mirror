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
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_DIMENSIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_DIMENSIONS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticTextDefPropDim;
		wxStaticLine* m_staticline1;
		wxStaticText* m_lblDimensionUnits;
		wxChoice* m_dimensionUnits;
		wxStaticText* m_lblTextPositionMode;
		wxChoice* m_dimensionTextPositionMode;
		wxStaticText* m_lblDimensionUnitsFormat;
		wxChoice* m_dimensionUnitsFormat;
		wxCheckBox* m_dimensionTextKeepAligned;
		wxStaticText* m_lblDimensionPrecision;
		wxChoice* m_dimensionPrecision;
		wxStaticText* m_lblArrowLength;
		wxTextCtrl* m_dimensionArrowLength;
		wxStaticText* m_arrowLengthUnits;
		wxCheckBox* m_dimensionSuppressZeroes;
		wxStaticText* m_lblExtensionOffset;
		wxTextCtrl* m_dimensionExtensionOffset;
		wxStaticText* m_dimensionExtensionOffsetUnits;

	public:

		PANEL_SETUP_DIMENSIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_DIMENSIONS_BASE();

};

