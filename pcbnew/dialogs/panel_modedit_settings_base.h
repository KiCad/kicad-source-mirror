///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_MODEDIT_SETTINGS_BASE_H__
#define __PANEL_MODEDIT_SETTINGS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

#define wxID_POLAR_CTRL 1000
#define wxID_UNITS 1001
#define wxID_SEGMENTS45 1002

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_MODEDIT_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_MODEDIT_SETTINGS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_UnitsSelection;
		wxCheckBox* m_MagneticPads;
		wxCheckBox* m_Segments_45_Only_Ctrl;
	
	public:
		
		PANEL_MODEDIT_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_MODEDIT_SETTINGS_BASE();
	
};

#endif //__PANEL_MODEDIT_SETTINGS_BASE_H__
