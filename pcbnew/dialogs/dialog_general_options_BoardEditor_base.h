///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  6 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GENERAL_OPTIONS_BOARDEDITOR_BASE_H__
#define __DIALOG_GENERAL_OPTIONS_BOARDEDITOR_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
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
			wxID_POLAR_CTRL = 1000,
			wxID_UNITS,
			wxID_CURSOR_SHAPE,
			wxID_DRC_ONOFF,
			wxID_GENERAL_RATSNEST,
			wxID_RATSNEST_MODULE,
			wxID_TRACK_AUTODEL,
			wxID_TRACKS45,
			wxID_SEGMENTS45,
			wxID_MAGNETIC_TRACKS,
			wxID_MIDDLEBUTTONPAN,
			wxID_AUTOPAN
		};
		
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_UnitsSelection;
		wxRadioBox* m_CursorShape;
		wxStaticText* m_staticTextmaxlinks;
		wxSpinCtrl* m_MaxShowLinks;
		wxStaticText* m_staticTextautosave;
		wxSpinCtrl* m_SaveTime;
		wxStaticText* m_staticTextRotationAngle;
		wxTextCtrl* m_RotationAngle;
		wxCheckBox* m_DrcOn;
		wxCheckBox* m_ShowGlobalRatsnest;
		wxCheckBox* m_ShowModuleRatsnest;
		wxCheckBox* m_TrackAutodel;
		wxCheckBox* m_Track_45_Only_Ctrl;
		wxCheckBox* m_Segments_45_Only_Ctrl;
		wxCheckBox* m_Track_DoubleSegm_Ctrl;
		wxRadioBox* m_MagneticPadOptCtrl;
		wxRadioBox* m_MagneticTrackOptCtrl;
		wxCheckBox* m_ZoomNoCenterOpt;
		wxCheckBox* m_MiddleButtonPANOpt;
		wxCheckBox* m_OptMiddleButtonPanLimited;
		wxCheckBox* m_AutoPANOpt;
		wxCheckBox* m_UseOldZoneFillingAlgo;
		wxCheckBox* m_DumpZonesWhenFilling;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnMiddleBtnPanEnbl( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("General Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GENERALOPTIONS_BOARDEDITOR_BASE();
	
};

#endif //__DIALOG_GENERAL_OPTIONS_BOARDEDITOR_BASE_H__
