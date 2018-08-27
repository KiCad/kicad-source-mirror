///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Aug  2 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE_H__
#define __PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE : public wxPanel 
{
	private:
	
	protected:
		WX_GRID* m_grid;
		wxBitmapButton* m_addFieldButton;
		wxBitmapButton* m_deleteFieldButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSizeGrid( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString ); 
		~PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE();
	
};

#endif //__PANEL_EESCHEMA_TEMPLATE_FIELDNAMES_BASE_H__
