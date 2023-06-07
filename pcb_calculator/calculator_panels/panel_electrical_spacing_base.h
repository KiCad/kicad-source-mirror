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
#include "panel_electrical_spacing_ipc2221.h"
#include "panel_electrical_spacing_iec60664.h"
#include "calculator_panels/calculator_panel.h"
#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/notebook.h>
#include <wx/sizer.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_ELECTRICAL_SPACING_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_ELECTRICAL_SPACING_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxNotebook* m_notebook1;
		PANEL_ELECTRICAL_SPACING_IPC2221* m_IPC2221;
		PANEL_ELECTRICAL_SPACING_IEC60664* m_IEC60664;

	public:

		PANEL_ELECTRICAL_SPACING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_ELECTRICAL_SPACING_BASE();

};

