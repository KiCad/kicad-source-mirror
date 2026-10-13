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
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GIT_BRANCHES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_BRANCHES_BASE : public wxPanel
{
	private:

	protected:
		wxSearchCtrl* m_branchSearch;
		wxSplitterWindow* m_branchesSplitter;
		wxPanel* m_branchListPanel;
		wxListCtrl* m_branchesList;
		wxStaticText* m_branchCount;
		wxPanel* m_branchDetail;
		wxStaticText* m_branchTitle;
		wxStaticText* m_branchCommit;
		wxStaticText* m_branchAuthor;
		wxTextCtrl* m_branchMessage;
		wxStaticText* m_branchSync;
		wxButton* m_newBranchButton;
		wxButton* m_deleteBranchButton;
		wxButton* m_forcePushButton;

	public:

		PANEL_GIT_BRANCHES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_BRANCHES_BASE();

};

