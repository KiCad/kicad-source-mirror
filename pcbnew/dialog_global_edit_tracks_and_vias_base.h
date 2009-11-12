///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_global_edit_tracks_and_vias_base__
#define __dialog_global_edit_tracks_and_vias_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CURRENT_VALUES_TO_CURRENT_NET 1000
#define ID_NETCLASS_VALUES_TO_CURRENT_NET 1001
#define ID_ALL_TRACKS_VIAS 1002
#define ID_ALL_VIAS 1003
#define ID_ALL_TRACKS 1004

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText12;
		
		wxStaticText* m_CurrentNetText;
		wxStaticText* m_CurrentNetName;
		wxStaticText* m_CurrentNetclassText;
		wxStaticText* m_CurrentNetclassName;
		
		wxGrid* m_gridDisplayCurrentSettings;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText11;
		
		wxRadioButton* m_Net2CurrValueButton;
		wxRadioButton* m_NetUseNetclassValueButton;
		wxRadioButton* m_radioBtn3;
		wxRadioButton* m_radioBtn4;
		wxRadioButton* m_radioBtn5;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSelectionClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Global Edition of Tracks and Vias"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 711,376 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE();
	
};

#endif //__dialog_global_edit_tracks_and_vias_base__
