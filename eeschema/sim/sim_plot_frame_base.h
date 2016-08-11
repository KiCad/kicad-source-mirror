///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SIM_PLOT_FRAME_BASE_H__
#define __SIM_PLOT_FRAME_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
class KIWAY_PLAYER;
class SIM_PLOT_PANEL;

#include "kiway_player.h"
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/aui/aui.h>
#include <wx/aui/auibar.h>
#include <wx/panel.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class SIM_PLOT_FRAME_BASE
///////////////////////////////////////////////////////////////////////////////
class SIM_PLOT_FRAME_BASE : public KIWAY_PLAYER
{
	private:
	
	protected:
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxAuiToolBar* m_auiToolBar1;
		wxAuiToolBarItem* m_toolZoomIn; 
		wxSplitterWindow* m_splitter1;
		SIM_PLOT_PANEL* m_plotPanel;
		wxPanel* m_panel3;
		wxRichTextCtrl* m_simConsole;
	
	public:
		SIM_PLOT_FRAME_BASE( KIWAY* aKiway, wxWindow* aParent );
		~SIM_PLOT_FRAME_BASE();
		
		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 700 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitter1OnIdle ), NULL, this );
		}
	
};


#endif //__SIM_PLOT_FRAME_BASE_H__
