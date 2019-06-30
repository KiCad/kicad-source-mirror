///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE_H__
#define __PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxBoxSizer* m_galOptionsSizer;
		wxStaticText* m_busWidthLabel;
		wxTextCtrl* m_busWidthCtrl;
		wxStaticText* m_busWidthUnits;
		wxStaticText* m_wireWidthLabel;
		wxTextCtrl* m_wireWidthCtrl;
		wxStaticText* m_wireWidthUnits;
		wxStaticText* m_jctSizeLabel;
		wxTextCtrl* m_jctSizeCtrl;
		wxStaticText* m_jctSizeUnits;
		wxStaticText* m_staticText26;
		wxChoice* m_choiceSeparatorRefId;
		wxCheckBox* m_checkShowHiddenPins;
		wxCheckBox* m_checkPageLimits;
	
	public:
		
		PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE();
	
};

#endif //__PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE_H__
