///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_MODEDIT_DEFAULTS_BASE_H__
#define __PANEL_MODEDIT_DEFAULTS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class TEXT_CTRL_EVAL;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/choice.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_MODEDIT_DEFAULTS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_MODEDIT_DEFAULTS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxStaticText* m_lineWidthLabel;
		TEXT_CTRL_EVAL* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_textThickLabel;
		TEXT_CTRL_EVAL* m_textThickCtrl;
		wxStaticText* m_textThickUnits;
		wxStaticText* m_textHeightLabel;
		TEXT_CTRL_EVAL* m_textHeightCtrl;
		wxStaticText* m_textHeightUnits;
		wxStaticText* m_textWidthLabel;
		TEXT_CTRL_EVAL* m_textWidthCtrl;
		wxStaticText* m_textWidthUnits;
		wxStaticText* m_staticTextRef;
		wxTextCtrl* m_textCtrlRefText;
		wxChoice* m_choiceLayerReference;
		wxChoice* m_choiceVisibleReference;
		wxStaticText* m_staticTextValue;
		wxTextCtrl* m_textCtrlValueText;
		wxChoice* m_choiceLayerValue;
		wxChoice* m_choiceVisibleValue;
		wxStaticText* m_staticTextInfo;
	
	public:
		
		PANEL_MODEDIT_DEFAULTS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_MODEDIT_DEFAULTS_BASE();
	
};

#endif //__PANEL_MODEDIT_DEFAULTS_BASE_H__
