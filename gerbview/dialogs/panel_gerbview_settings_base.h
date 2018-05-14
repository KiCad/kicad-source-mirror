///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_GERBVIEW_SETTINGS_BASE_H__
#define __PANEL_GERBVIEW_SETTINGS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GERBVIEW_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GERBVIEW_SETTINGS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxBoxSizer* m_UpperSizer;
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_BoxUnits;
		wxRadioBox* m_PageSize;
		wxCheckBox* m_ShowPageLimitsOpt;
	
	public:
		
		PANEL_GERBVIEW_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_GERBVIEW_SETTINGS_BASE();
	
};

#endif //__PANEL_GERBVIEW_SETTINGS_BASE_H__
