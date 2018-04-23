///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 19 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GENERAL_OPTIONS_BOARDEDITOR_BASE_H__
#define __DIALOG_GENERAL_OPTIONS_BOARDEDITOR_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/stepped_slider.h"
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			wxID_AUTOPAN = 1000,
			wxID_POLAR_CTRL,
			wxID_UNITS,
			wxID_GENERAL_RATSNEST,
			wxID_SEGMENTS45,
			wxID_MAGNETIC_TRACKS,
			wxID_DRC_ONOFF,
			wxID_TRACK_AUTODEL,
			wxID_TRACKS45
		};
		
		wxStaticText* m_staticTextautosave;
		wxSpinCtrl* m_SaveTime;
		wxStaticText* m_staticText1;
		STEPPED_SLIDER* m_scaleSlider;
		wxStaticText* m_staticText2;
		wxCheckBox* m_scaleAuto;
		wxCheckBox* m_checkBoxIconsInMenus;
		wxCheckBox* m_ZoomCenterOpt;
		wxCheckBox* m_MousewheelPANOpt;
		wxCheckBox* m_AutoPANOpt;
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_UnitsSelection;
		wxCheckBox* m_ShowGlobalRatsnest;
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
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnScaleSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnScaleAuto( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("General Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE();
	
};

#endif //__DIALOG_GENERAL_OPTIONS_BOARDEDITOR_BASE_H__
