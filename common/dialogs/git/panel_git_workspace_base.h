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
class PANEL_MDW_VERSIONS;

#include "widgets/wx_dataviewctrl.h"
#include "widgets/wx_splitter_window.h"
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/dataview.h>
#include <wx/stattext.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GIT_WORKSPACE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_WORKSPACE_BASE : public wxPanel
{
	private:

	protected:
		wxTextCtrl* m_search;
		WX_SPLITTER_WINDOW* m_workspaceSplitter;
		wxPanel* m_itemListPanel;
		WX_DATAVIEWCTRL* m_itemsTree;
		wxStaticText* m_itemCount;
		wxHyperlinkCtrl* m_moreLink;
		wxHyperlinkCtrl* m_retryLink;
		wxPanel* m_itemDetail;
		wxStaticText* m_itemTitle;
		wxFlexGridSizer* m_detailRows;
		PANEL_MDW_VERSIONS* m_versions;
		wxBoxSizer* m_actionSizer;
		wxButton* m_connectLibraryButton;

	public:

		PANEL_GIT_WORKSPACE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_WORKSPACE_BASE();

};

