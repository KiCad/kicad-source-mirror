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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SNAPPING_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SNAPPING_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_gridSnappingLabel;
		wxStaticLine* m_gridSnappingLine;
		wxStaticText* m_gridSnapLabel;
		wxChoice* m_gridSnapChoice;
		wxStaticText* m_objectSnappingLabel;
		wxStaticLine* m_objectSnappingLine;
		wxBoxSizer* m_sizerObjectSnapping;
		wxStaticText* m_padSnapLabel;
		wxChoice* m_padSnapChoice;
		wxStaticText* m_trackSnapLabel;
		wxChoice* m_trackSnapChoice;
		wxStaticText* m_graphicsSnapLabel;
		wxChoice* m_graphicsSnapChoice;
		wxCheckBox* m_snapAllLayers;
		wxStaticText* m_snapInferenceLabel;
		wxStaticLine* m_snapInferenceLine;
		wxBoxSizer* m_sizerSnapInference;
		wxCheckBox* m_snapObjectGeometry;
		wxCheckBox* m_snapConstructionExtensions;
		wxCheckBox* m_snapTangentNormal;
		wxCheckBox* m_snapAlignmentDistribution;

	public:

		PANEL_SNAPPING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SNAPPING_BASE();

};

