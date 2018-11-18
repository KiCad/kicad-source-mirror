///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 11 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_PCBNEW_SETTINGS_BASE_H__
#define __PANEL_PCBNEW_SETTINGS_BASE_H__

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
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCBNEW_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCBNEW_SETTINGS_BASE : public wxPanel 
{
	private:
	
	protected:
		enum
		{
			wxID_POLAR_CTRL = 1000,
			wxID_UNITS,
			wxID_SEGMENTS45,
			wxID_MAGNETIC_TRACKS,
			wxID_DRC_ONOFF,
			wxID_TRACK_AUTODEL,
			wxID_TRACKS45
		};
		
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_UnitsSelection;
		wxCheckBox* m_Show_Page_Limits;
		wxCheckBox* m_Segments_45_Only_Ctrl;
		wxCheckBox* m_UseEditKeyForWidth;
		wxCheckBox* m_dragSelects;
		wxStaticText* m_staticTextRotationAngle;
		wxTextCtrl* m_RotationAngle;
		wxRadioBox* m_MagneticPadOptCtrl;
		wxRadioBox* m_MagneticTrackOptCtrl;
		wxCheckBox* m_DrcOn;
		wxCheckBox* m_TrackAutodel;
		wxCheckBox* m_Track_45_Only_Ctrl;
		wxCheckBox* m_Track_DoubleSegm_Ctrl;
	
	public:
		
		PANEL_PCBNEW_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_PCBNEW_SETTINGS_BASE();
	
};

#endif //__PANEL_PCBNEW_SETTINGS_BASE_H__
