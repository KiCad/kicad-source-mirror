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
/// Class PANEL_GIT_REMOTES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GIT_REMOTES_BASE : public wxPanel
{
	private:

	protected:
		wxSplitterWindow* m_remotesSplitter;
		wxPanel* m_remoteListPanel;
		wxListCtrl* m_remotesList;
		wxStaticText* m_remoteCount;
		wxPanel* m_remoteDetail;
		wxStaticText* m_remoteTitle;
		wxTextCtrl* m_remoteUrl;
		wxStaticText* m_remoteFetched;
		wxButton* m_editUrlButton;
		wxButton* m_removeRemoteButton;
		wxButton* m_addRemoteButton;

	public:

		PANEL_GIT_REMOTES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GIT_REMOTES_BASE();

};

