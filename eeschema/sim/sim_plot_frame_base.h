///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 24 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SIM_PLOT_FRAME_BASE_H__
#define __SIM_PLOT_FRAME_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
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
		wxPanel* m_panel31;
		wxPanel* m_panel61;
		wxAuiNotebook* m_plotNotebook;
		wxPanel* m_panel7;
		wxStaticText* m_staticText2;
		wxListBox* m_signals;
		wxStaticText* m_staticText21;
		wxListBox* m_signals1;
		wxButton* m_simulateBtn;
		wxButton* m_probeBtn;
		wxButton* m_tuneBtn;
		wxPanel* m_panel3;
		wxRichTextCtrl* m_simConsole;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onNewPlot( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSignalDblClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSimulate( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPlaceProbe( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTune( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation Workbook"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1280,900 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL, const wxString& name = wxT("SIM_PLOT_FRAME") );
		
		~SIM_PLOT_FRAME_BASE();
	
};

#endif //__SIM_PLOT_FRAME_BASE_H__
