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
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
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
		wxMenuBar* m_mainMenu;
		wxMenu* m_fileMenu;
		wxMenu* m_viewMenu;
		wxNotebook* m_plotNotebook;
		wxStaticText* m_staticText2;
		wxListBox* m_signals;
		wxStaticText* m_staticText21;
		wxListCtrl* m_cursors;
		wxButton* m_simulateBtn;
		wxButton* m_probeBtn;
		wxButton* m_tuneBtn;
		wxRichTextCtrl* m_simConsole;
		
		// Virtual event handlers, overide them in your derived class
		virtual void menuNewPlot( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuOpenWorkbook( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuSaveWorkbook( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuExit( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuZoomIn( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuZoomOut( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuZoomFit( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowGrid( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowGridUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void menuShowLegend( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowLegendUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void menuShowCoords( wxCommandEvent& event ) { event.Skip(); }
		virtual void menuShowCoordsUpdate( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onPlotChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void onSignalDblClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSignalRClick( wxMouseEvent& event ) { event.Skip(); }
		virtual void onSimulate( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPlaceProbe( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTune( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		SIM_PLOT_FRAME_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Simulation Workbook"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1280,900 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL, const wxString& name = wxT("SIM_PLOT_FRAME") );
		
		~SIM_PLOT_FRAME_BASE();
	
};

#endif //__SIM_PLOT_FRAME_BASE_H__
