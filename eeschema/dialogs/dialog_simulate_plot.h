///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SIMULATE_PLOT_H__
#define __DIALOG_SIMULATE_PLOT_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
class SIM_PLOT_PANEL;

#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gdicmn.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class SIM_PLOT_FRAME
///////////////////////////////////////////////////////////////////////////////
class SIM_PLOT_FRAME : public wxFrame 
{
	private:
	
	protected:
		wxAuiToolBar* m_auiToolBar1;
		wxAuiToolBarItem* m_toolZoomIn; 
		wxSplitterWindow* m_splitter1;
		SIM_PLOT_PANEL* m_plotPanel;
		wxPanel* m_panel3;
		wxRichTextCtrl* m_simConsole;
	
	public:
		
		SIM_PLOT_FRAME( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Spice Simulation"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 548,434 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~SIM_PLOT_FRAME();
		
		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 0 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME::m_splitter1OnIdle ), NULL, this );
		}
	
};

#endif //__DIALOG_SIMULATE_PLOT_H__
