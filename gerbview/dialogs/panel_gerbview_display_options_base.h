///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE_H__
#define __PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxBoxSizer* m_UpperSizer;
		wxBoxSizer* m_galOptionsSizer;
		wxCheckBox* m_OptDisplayDCodes;
		wxCheckBox* m_OptDisplayFlashedItems;
		wxCheckBox* m_OptDisplayLines;
		wxCheckBox* m_OptDisplayPolygons;
	
	public:
		
		PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE();
	
};

#endif //__PANEL_GERBVIEW_DISPLAY_OPTIONS_BASE_H__
