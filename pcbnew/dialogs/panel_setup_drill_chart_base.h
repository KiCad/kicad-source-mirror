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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checklst.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_DRILL_CHART_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_DRILL_CHART_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_groupByLabel;
		wxCheckListBox* m_groupByList;
		wxStaticText* m_markPolicyLabel;
		wxChoice* m_markPolicyCtrl;
		wxStaticText* m_markPolicyPad;
		wxStaticText* m_symbolSizeLabel;
		wxTextCtrl* m_symbolSizeCtrl;
		wxStaticText* m_symbolSizeUnits;
		wxStaticText* m_symbolWidthLabel;
		wxTextCtrl* m_symbolWidthCtrl;
		wxStaticText* m_symbolWidthUnits;
		wxCheckBox* m_freezeAssignments;
		wxButton* m_editGroupsButton;

		// Virtual event handlers, override them in your derived class
		virtual void onEditGroups( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_DRILL_CHART_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_DRILL_CHART_BASE();

};

