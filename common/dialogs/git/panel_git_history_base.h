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
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dataview.h>
#include <wx/panel.h>
#include <wx/statbmp.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GIT_HISTORY_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_HISTORY_BASE : public wxPanel
{
	private:

	protected:
		wxSearchCtrl* m_historySearch;
		wxChoice* m_historyBranch;
		wxCheckBox* m_historyShowRefs;
		wxBoxSizer* m_historyPathRow;
		wxStaticText* m_historyPathLabel;
		wxButton* m_showAllButton;
		wxSplitterWindow* m_historySplitter;
		wxPanel* m_historyListPanel;
		wxDataViewListCtrl* m_historyList;
		wxPanel* m_commitDetail;
		wxStaticText* m_commitTitle;
		wxStaticBitmap* m_commitAvatar;
		wxStaticText* m_commitAuthor;
		wxTextCtrl* m_commitMessage;
		wxStaticText* m_commitStats;
		wxStaticText* m_commitFileCount;
		wxStaticText* m_commitAdded;
		wxStaticText* m_commitRemoved;
		wxSplitterWindow* m_detailSplitter;
		wxPanel* m_commitFilesPanel;
		wxListCtrl* m_commitFiles;

	public:

		PANEL_GIT_HISTORY_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_HISTORY_BASE();

};

