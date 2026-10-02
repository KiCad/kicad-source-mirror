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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <widgets/file_checklist_ctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class GIT_COMMIT_PANE_BASE
///////////////////////////////////////////////////////////////////////////////
class GIT_COMMIT_PANE_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* m_branchRow;
		wxChoice* m_branchChoice;
		wxTextCtrl* m_summary;
		wxStaticText* m_summaryCount;
		wxTextCtrl* m_description;
		wxStaticBoxSizer* m_optionsSizer;
		wxCheckBox* m_amendCheckbox;
		wxCheckBox* m_signOffCheckbox;
		wxCheckBox* m_newBranchCheckbox;
		wxTextCtrl* m_newBranchName;
		wxBoxSizer* m_changesRow;
		wxStaticText* m_changesLabel;
		wxButton* m_showDiffButton;
		wxStaticText* m_emptyLabel;
		FILE_CHECKLIST_CTRL* m_checklist;
		wxButton* m_commitOnlyButton;
		wxButton* m_commitButton;
		wxButton* m_pullButton;
		wxButton* m_advancedButton;

	public:

		GIT_COMMIT_PANE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,360 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~GIT_COMMIT_PANE_BASE();

};

