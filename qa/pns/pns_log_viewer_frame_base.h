///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Feb  6 2021)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/treelist.h>
#include <wx/statusbr.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PNS_LOG_VIEWER_FRAME_BASE
///////////////////////////////////////////////////////////////////////////////
class PNS_LOG_VIEWER_FRAME_BASE : public wxFrame
{
	private:

	protected:
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxBoxSizer* m_mainSizer;
		wxStaticText* m_rewindText;
		wxButton* m_rewindLeft;
		wxSlider* m_rewindSlider;
		wxButton* m_rewindRight;
		wxTextCtrl* m_rewindPos;
		wxBoxSizer* m_viewSizer;
		wxTreeListCtrl* m_itemList;
		wxStatusBar* m_statusBar;

		// Virtual event handlers, override them in your derived class
		virtual void onReload( wxCommandEvent& event ) { event.Skip(); }
		virtual void onExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBtnRewindLeft( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRewindScroll( wxScrollEvent& event ) { event.Skip(); }
		virtual void onBtnRewindRight( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRewindCountText2( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRewindCountText( wxCommandEvent& event ) { event.Skip(); }


	public:

		PNS_LOG_VIEWER_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("P&S Log Viewer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~PNS_LOG_VIEWER_FRAME_BASE();

};

