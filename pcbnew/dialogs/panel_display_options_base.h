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
#include "widgets/resettable_panel.h"
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/simplebook.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_DISPLAY_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		enum
		{
			ID_SHOW_CLEARANCE = 1000
		};

		wxBoxSizer* m_galOptionsSizer;
		wxSimplebook* m_optionsBook;
		wxStaticText* m_annotationsLabel;
		wxStaticLine* m_staticline1;
		wxRadioBox* m_ShowNetNamesOption;
		wxCheckBox* m_OptDisplayPadNumber;
		wxStaticText* m_clearanceLabel;
		wxStaticLine* m_staticline2;
		wxRadioBox* m_OptDisplayTracksClearance;
		wxCheckBox* m_OptDisplayPadClearence;
		wxStaticText* m_crossProbingLabel;
		wxStaticLine* m_staticline3;
		wxCheckBox* m_checkCrossProbeOnSelection;
		wxCheckBox* m_checkCrossProbeCenter;
		wxCheckBox* m_checkCrossProbeZoom;
		wxCheckBox* m_checkCrossProbeAutoHighlight;
		wxCheckBox* m_live3Drefresh;

	public:

		PANEL_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_DISPLAY_OPTIONS_BASE();

};

