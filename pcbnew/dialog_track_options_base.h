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
#include <wx/listbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
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
		void _wxFB_OnButtonAddViaSizeClick( wxCommandEvent& event ){ OnButtonAddViaSizeClick( event ); }
		void _wxFB_OnButtonDeleteViaSizeClick( wxCommandEvent& event ){ OnButtonDeleteViaSizeClick( event ); }
		void _wxFB_OnButtonAddTrackSizeClick( wxCommandEvent& event ){ OnButtonAddTrackSizeClick( event ); }
		void _wxFB_OnButtonDeleteTrackSizeClick( wxCommandEvent& event ){ OnButtonDeleteTrackSizeClick( event ); }
		void _wxFB_OnButtonCancelClick( wxCommandEvent& event ){ OnButtonCancelClick( event ); }
		void _wxFB_OnButtonOkClick( wxCommandEvent& event ){ OnButtonOkClick( event ); }
		
	
	protected:
		enum
		{
			wxID_ADD_VIA_SIZE = 1000,
			wxID_DELETED_WIA_SIEZ,
			wxID_ADD_TRACK_WIDTH,
			wxID_DELETED_TRACK_WIDTH,
		};
		
		wxListBox* m_ViaSizeListCtrl;
		wxButton* m_buttonAddViasSize;
		wxButton* m_button4;
		wxStaticText* m_ViaAltDrillValueTitle;
		wxTextCtrl* m_OptCustomViaDrill;
		wxListBox* m_TrackWidthListCtrl;
		wxButton* m_buttonAddTrackSize;
		wxButton* m_buttonDeleteTrackWidth;
		
		wxStaticText* m_MaskClearanceTitle;
		wxTextCtrl* m_OptMaskMargin;
		wxStdDialogButtonSizer* m_sdbButtonsSizer;
		wxButton* m_sdbButtonsSizerOK;
		wxButton* m_sdbButtonsSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnButtonAddViaSizeClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonDeleteViaSizeClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonAddTrackSizeClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonDeleteTrackSizeClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_TRACKS_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Tracks and Vias Sizes"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,351 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_TRACKS_OPTIONS_BASE();
	
};

#endif //__dialog_track_options_base__
