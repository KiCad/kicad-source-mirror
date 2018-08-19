///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_FP_LIB_TABLE_BASE_H__
#define __PANEL_FP_LIB_TABLE_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/aui/auibook.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/statbox.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FP_LIB_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FP_LIB_TABLE_BASE : public wxPanel 
{
	private:
	
	protected:
		wxAuiNotebook* m_auinotebook;
		wxPanel* m_global_panel;
		wxStaticText* m_staticText3;
		wxStaticText* m_GblTableFilename;
		WX_GRID* m_global_grid;
		wxPanel* m_project_panel;
		wxStaticText* m_staticText4;
		wxStaticText* m_PrjTableFilename;
		WX_GRID* m_project_grid;
		wxBitmapButton* m_append_button;
		wxBitmapButton* m_browse_button;
		wxBitmapButton* m_move_up_button;
		wxBitmapButton* m_move_down_button;
		wxBitmapButton* m_delete_button;
		WX_GRID* m_path_subs_grid;
		
		// Virtual event handlers, overide them in your derived class
		virtual void pageChangedHandler( wxAuiNotebookEvent& event ) { event.Skip(); }
		virtual void appendRowHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void browseLibrariesHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveUpHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void moveDownHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void deleteRowHandler( wxCommandEvent& event ) { event.Skip(); }
		virtual void onSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		
	
	public:
		
		PANEL_FP_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_FP_LIB_TABLE_BASE();
	
};

#endif //__PANEL_FP_LIB_TABLE_BASE_H__
