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
#include <wx/listctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
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
/// Class PANEL_GIT_TAGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_TAGS_BASE : public wxPanel
{
	private:

	protected:
		wxSplitterWindow* m_tagsSplitter;
		wxPanel* m_tagListPanel;
		wxListCtrl* m_tagsList;
		wxStaticText* m_tagCount;
		wxPanel* m_tagDetail;
		wxStaticText* m_tagTitle;
		wxStaticText* m_tagCommit;
		wxStaticText* m_tagAuthor;
		wxStaticText* m_tagWhen;
		wxTextCtrl* m_tagMessage;
		wxButton* m_checkoutTagButton;
		wxButton* m_deleteTagButton;
		wxButton* m_createTagButton;

	public:

		PANEL_GIT_TAGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_TAGS_BASE();

};

