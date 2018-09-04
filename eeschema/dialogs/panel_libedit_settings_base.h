///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PANEL_LIBEDIT_SETTINGS_BASE_H__
#define __PANEL_LIBEDIT_SETTINGS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_LIBEDIT_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_LIBEDIT_SETTINGS_BASE : public wxPanel 
{
	private:
	
	protected:
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_pinLengthLabel;
		wxTextCtrl* m_pinLengthCtrl;
		wxStaticText* m_pinLengthUnits;
		wxStaticText* m_pinNumSizeLabel;
		wxTextCtrl* m_pinNumSizeCtrl;
		wxStaticText* m_pinNumSizeUnits;
		wxStaticText* m_pinNameSizeLabel;
		wxTextCtrl* m_pinNameSizeCtrl;
		wxStaticText* m_pinNameSizeUnits;
		wxCheckBox* m_checkShowPinElectricalType;
		wxStaticText* m_hPitchLabel;
		wxTextCtrl* m_hPitchCtrl;
		wxStaticText* m_hPitchUnits;
		wxStaticText* m_vPitchLabel;
		wxTextCtrl* m_vPitchCtrl;
		wxStaticText* m_vPitchUnits;
		wxStaticText* m_pinPitchLabel;
		wxChoice* m_choicePinDisplacement;
		wxStaticText* m_pinPitchUnis;
		wxStaticText* m_labelIncrementLabel;
		wxSpinCtrl* m_spinRepeatLabel;
	
	public:
		
		PANEL_LIBEDIT_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL ); 
		~PANEL_LIBEDIT_SETTINGS_BASE();
	
};

#endif //__PANEL_LIBEDIT_SETTINGS_BASE_H__
