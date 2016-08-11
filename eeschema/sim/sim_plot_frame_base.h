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
#include <wx/aui/auibook.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/richtext/richtextctrl.h>
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
		wxMenu* m_menu2;
		wxSplitterWindow* m_splitter1;
		wxPanel* m_panel31;
		wxSplitterWindow* m_splitter2;
		wxPanel* m_panel61;
		wxAuiNotebook* m_plotNotebook;
		wxPanel* m_panel7;
		wxStaticText* m_staticText2;
		wxListBox* m_signals;
		wxStaticText* m_staticText21;
		wxListBox* m_signals1;
		wxButton* m_button1;
		wxButton* m_button2;
		wxPanel* m_panel3;
		wxRichTextCtrl* m_simConsole;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onNewPlot( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		SIM_PLOT_FRAME_BASE( KIWAY* aKiway, wxWindow* aParent );
		~SIM_PLOT_FRAME_BASE();
		
		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 700 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitter1OnIdle ), NULL, this );
		}
		
		void m_splitter2OnIdle( wxIdleEvent& )
		{
			m_splitter2->SetSashPosition( 0 );
			m_splitter2->Disconnect( wxEVT_IDLE, wxIdleEventHandler( SIM_PLOT_FRAME_BASE::m_splitter2OnIdle ), NULL, this );
		}
	
};

#endif //__SIM_PLOT_FRAME_BASE_H__
