///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/treelist.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
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
		wxMenu* m_menuFile;
		wxMenu* m_menuView;
		wxBoxSizer* m_mainSizer;
		wxFlexGridSizer* m_topBarSizer;
		wxStaticText* m_rewindText;
		wxButton* m_rewindLeft;
		wxSlider* m_rewindSlider;
		wxButton* m_rewindRight;
		wxTextCtrl* m_rewindPos;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_filterString;
		wxStaticText* m_algoStatus;
		wxStaticText* m_ideLabel;
		wxChoice* m_ideChoice;
		wxSplitterWindow* m_mainSplitter;
		wxPanel* m_panelProps;
		wxNotebook* m_propsNotebook;
		wxPanel* m_panelListView;
		wxTreeListCtrl* m_itemList;
		wxPanel* m_panelConsole;
		wxTextCtrl* m_consoleText;
		wxStatusBar* m_statusBar;

		// Virtual event handlers, override them in your derived class
		virtual void onOpen( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSaveAs( wxCommandEvent& event ) { event.Skip(); }
		virtual void onExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onShowRPIsChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onShowThinLinesChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onShowVerticesChecked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBtnRewindLeft( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRewindScroll( wxScrollEvent& event ) { event.Skip(); }
		virtual void onBtnRewindRight( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRewindCountText2( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRewindCountText( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFilterText( wxCommandEvent& event ) { event.Skip(); }


	public:

		PNS_LOG_VIEWER_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("P&S Log Viewer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1045,574 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~PNS_LOG_VIEWER_FRAME_BASE();

		void m_mainSplitterOnIdle( wxIdleEvent& )
		{
			m_mainSplitter->SetSashPosition( 0 );
			m_mainSplitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PNS_LOG_VIEWER_FRAME_BASE::m_mainSplitterOnIdle ), NULL, this );
		}

};

