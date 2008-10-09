///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_copper_zones_frame__
#define __dialog_copper_zones_frame__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class dialog_copper_zone_frame
///////////////////////////////////////////////////////////////////////////////
class dialog_copper_zone_frame : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnInitDialog( wxInitDialogEvent& event ){ OnInitDialog( event ); }
		void _wxFB_OnOkButtonClick( wxCommandEvent& event ){ OnOkButtonClick( event ); }
		void _wxFB_OnButtonCancleClick( wxCommandEvent& event ){ OnButtonCancleClick( event ); }
		void _wxFB_OnNetSortingOptionSelected( wxCommandEvent& event ){ OnNetSortingOptionSelected( event ); }
		
	
	protected:
		enum
		{
			 ID_RADIOBOX_GRID_SELECTION = 1000,
			ID_RADIOBOX_OUTLINES_OPTION,
			ID_NET_SORTING_OPTION,
			ID_TEXTCTRL_NETNAMES_FILTER,
			ID_NETNAME_SELECTION,
			ID_LAYER_CHOICE,
		};
		
		wxRadioBox* m_GridCtrl;
		wxStaticText* m_ClearanceValueTitle;
		wxTextCtrl* m_ZoneClearanceCtrl;
		wxRadioBox* m_OutlineAppearanceCtrl;
		wxRadioBox* m_FillOpt;
		
		wxRadioBox* m_OrientEdgesOpt;
		
		wxButton* m_OkButton;
		wxButton* m_ButtonCancel;
		
		wxRadioBox* m_NetSortingOption;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_NetNameFilter;
		wxStaticText* m_staticText2;
		wxListBox* m_ListNetNameSelection;
		wxStaticText* m_staticText3;
		wxListBox* m_LayerSelectionCtrl;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnInitDialog( wxInitDialogEvent& event ){ event.Skip(); }
		virtual void OnOkButtonClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCancleClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnNetSortingOptionSelected( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		dialog_copper_zone_frame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 578,567 ), long style = wxDEFAULT_DIALOG_STYLE );
		~dialog_copper_zone_frame();
	
};

#endif //__dialog_copper_zones_frame__
