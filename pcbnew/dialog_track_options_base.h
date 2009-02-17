///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_track_options_base__
#define __dialog_track_options_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TRACKS_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TRACKS_OPTIONS_BASE : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnInitDialog( wxInitDialogEvent& event ){ OnInitDialog( event ); }
		void _wxFB_OnCheckboxAllowsMicroviaClick( wxCommandEvent& event ){ OnCheckboxAllowsMicroviaClick( event ); }
		void _wxFB_OnButtonOkClick( wxCommandEvent& event ){ OnButtonOkClick( event ); }
		void _wxFB_OnButtonCancelClick( wxCommandEvent& event ){ OnButtonCancelClick( event ); }
		
	
	protected:
		wxStaticText* m_ViaSizeTitle;
		wxTextCtrl* m_OptViaSize;
		wxStaticText* m_ViaDefaultDrillValueTitle;
		wxTextCtrl* m_OptViaDrill;
		wxStaticText* m_ViaAltDrillValueTitle;
		wxTextCtrl* m_OptCustomViaDrill;
		wxRadioBox* m_OptViaType;
		wxStaticText* m_MicroViaSizeTitle;
		wxTextCtrl* m_MicroViaSizeCtrl;
		wxStaticText* m_MicroViaDrillTitle;
		wxTextCtrl* m_MicroViaDrillCtrl;
		
		wxCheckBox* m_AllowMicroViaCtrl;
		wxStaticText* m_TrackWidthTitle;
		wxTextCtrl* m_OptTrackWidth;
		wxStaticText* m_TrackClearanceTitle;
		wxTextCtrl* m_OptTrackClearance;
		
		wxStaticText* m_MaskClearanceTitle;
		wxTextCtrl* m_OptMaskMargin;
		wxButton* m_buttonOK;
		wxButton* m_buttonCANCEL;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnCheckboxAllowsMicroviaClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_TRACKS_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Tracks and Vias Sizes"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_TRACKS_OPTIONS_BASE();
	
};

#endif //__dialog_track_options_base__
