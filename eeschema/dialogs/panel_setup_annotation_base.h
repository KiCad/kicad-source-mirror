///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_ANNOTATION_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_ANNOTATION_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxCheckBox* m_checkReuseRefdes;
		wxStaticText* m_unitsLabel;
		wxStaticLine* m_staticline2;
		wxStaticText* m_staticText3;
		wxChoice* m_choiceSeparatorRefId;

	public:

		PANEL_SETUP_ANNOTATION_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_ANNOTATION_BASE();

};

