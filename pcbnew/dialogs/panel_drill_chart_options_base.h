///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_generated_table_properties.h"
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checklst.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_DRILL_CHART_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_DRILL_CHART_OPTIONS_BASE : public GENERATED_TABLE_OPTIONS_PANEL
{
	private:

	protected:
		wxCheckBox* m_filterPlated;
		wxCheckBox* m_filterNonPlated;
		wxCheckBox* m_filterVias;
		wxCheckBox* m_filterSlots;
		wxCheckBox* m_filterBackdrills;
		wxCheckBox* m_filterCastellated;
		wxCheckBox* m_showTotals;
		wxCheckListBox* m_groupByList;
		wxButton* m_importTemplateButton;
		wxButton* m_exportTemplateButton;

		// Virtual event handlers, override them in your derived class
		virtual void onImportTemplate( wxCommandEvent& event ) { event.Skip(); }
		virtual void onExportTemplate( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_DRILL_CHART_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_DRILL_CHART_OPTIONS_BASE();

};

