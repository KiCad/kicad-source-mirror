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
class STD_BITMAP_BUTTON;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <widgets/file_checklist_ctrl.h>
#include <widgets/split_button.h>
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
		wxStaticText* m_branchName;
		wxChoice* m_recentMessages;
		STD_BITMAP_BUTTON* m_settingsButton;
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
		wxBoxSizer* m_trustRow;
		wxTextCtrl* m_trustCommand;
		wxButton* m_copyTrustCommand;
		wxButton* m_undoCommitButton;
		FILE_CHECKLIST_CTRL* m_checklist;
		SPLIT_BUTTON* m_commitButton;
		wxButton* m_pullButton;

	public:

		GIT_COMMIT_PANE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,360 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~GIT_COMMIT_PANE_BASE();

};

