///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_SETUP_FEATURE_CONSTRAINTS_BASE_H__
#define __PANEL_SETUP_FEATURE_CONSTRAINTS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_FEATURE_CONSTRAINTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_FEATURE_CONSTRAINTS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxCheckBox* m_OptAllowBlindBuriedVias;
		wxCheckBox* m_OptAllowMicroVias;
		wxCheckBox* m_OptRequireCourtyards;
		wxCheckBox* m_OptOverlappingCourtyards;
		wxStaticText* m_TrackMinWidthTitle;
		wxTextCtrl* m_TrackMinWidthCtrl;
		wxStaticText* m_TrackMinWidthUnits;
		wxStaticText* m_ViaMinTitle;
		wxTextCtrl* m_SetViasMinSizeCtrl;
		wxStaticText* m_ViaMinUnits;
		wxStaticText* m_ViaMinDrillTitle;
		wxTextCtrl* m_SetViasMinDrillCtrl;
		wxStaticText* m_ViaMinDrillUnits;
		wxStaticText* m_uviaMinSizeLabel;
		wxTextCtrl* m_uviaMinSizeCtrl;
		wxStaticText* m_uviaMinSizeUnits;
		wxStaticText* m_uviaMinDrillLabel;
		wxTextCtrl* m_uviaMinDrillCtrl;
		wxStaticText* m_uviaMinDrillUnits;
		wxStaticText* m_HoleToHoleTitle;
		wxTextCtrl* m_SetHoleToHoleCtrl;
		wxStaticText* m_HoleToHoleUnits;
	
	public:
		
		PANEL_SETUP_FEATURE_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_SETUP_FEATURE_CONSTRAINTS_BASE();
	
};

#endif //__PANEL_SETUP_FEATURE_CONSTRAINTS_BASE_H__
