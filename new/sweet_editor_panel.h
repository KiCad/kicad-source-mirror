///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __sweet_editor_panel__
#define __sweet_editor_panel__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/scrolwin.h>
#include <sch_canvas.h>
#include <wx/splitter.h>
#include <wx/panel.h>
#include <wx/html/htmlwin.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class SWEET_EDITOR_PANEL
///////////////////////////////////////////////////////////////////////////////
class SWEET_EDITOR_PANEL : public wxPanel 
{
	private:
	
	protected:
		wxSplitterWindow* m_top_bottom;
		wxPanel* m_panel9;
		wxSplitterWindow* m_splitter3;
		wxScrolledWindow* m_scrolledTextWindow;
		wxTextCtrl* m_sweet_scroll_window;
		wxScrolledWindow* m_gal_scrolled_window;
		SCH::CANVAS* m_gal;
		wxScrolledWindow* m_scrolledWindow3;
		wxHtmlWindow* m_htmlWin2;
	
	public:
		
		SWEET_EDITOR_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL ); 
		~SWEET_EDITOR_PANEL();
		
		void m_top_bottomOnIdle( wxIdleEvent& )
		{
			m_top_bottom->SetSashPosition( 0 );
			m_top_bottom->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SWEET_EDITOR_PANEL::m_top_bottomOnIdle ), NULL, this );
		}
		
		void m_splitter3OnIdle( wxIdleEvent& )
		{
			m_splitter3->SetSashPosition( 0 );
			m_splitter3->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SWEET_EDITOR_PANEL::m_splitter3OnIdle ), NULL, this );
		}
	
};

#endif //__sweet_editor_panel__
