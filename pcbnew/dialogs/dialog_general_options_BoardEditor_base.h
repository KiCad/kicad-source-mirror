///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  6 2011)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_general_options_BoardEditor_base__
#define __dialog_general_options_BoardEditor_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogGeneralOptionsBoardEditor_base
///////////////////////////////////////////////////////////////////////////////
class DialogGeneralOptionsBoardEditor_base : public wxDialog 
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
			wxID_AUTOPAN,
			wxID_MAGNETIC_TRACKS,
		};
		
		wxRadioBox* m_PolarDisplay;
		wxRadioBox* m_UnitsSelection;
		wxRadioBox* m_CursorShape;
		wxStaticText* m_staticTextmaxlinks;
		wxSpinCtrl* m_MaxShowLinks;
		wxStaticText* m_staticTextautosave;
		wxSpinCtrl* m_SaveTime;
		wxStaticText* m_staticTextRotationAngle;
		wxChoice* m_RotationAngle;
		wxCheckBox* m_DrcOn;
		wxCheckBox* m_ShowGlobalRatsnest;
		wxCheckBox* m_ShowModuleRatsnest;
		wxCheckBox* m_TrackAutodel;
		wxCheckBox* m_Track_45_Only_Ctrl;
		wxCheckBox* m_Segments_45_Only_Ctrl;
		wxCheckBox* m_AutoPANOpt;
		wxCheckBox* m_Track_DoubleSegm_Ctrl;
		wxRadioBox* m_MagneticPadOptCtrl;
		wxRadioBox* m_MagneticTrackOptCtrl;
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DialogGeneralOptionsBoardEditor_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("General settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 585,280 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DialogGeneralOptionsBoardEditor_base();
	
};

#endif //__dialog_general_options_BoardEditor_base__
