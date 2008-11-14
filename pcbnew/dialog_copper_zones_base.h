///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_copper_zones_base__
#define __dialog_copper_zones_base__

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
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class dialog_copper_zone_base
///////////////////////////////////////////////////////////////////////////////
class dialog_copper_zone_base : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnInitDialog( wxInitDialogEvent& event ){ OnInitDialog( event ); }
		void _wxFB_OnPadsInZoneClick( wxCommandEvent& event ){ OnPadsInZoneClick( event ); }
		void _wxFB_ExportSetupToOtherCopperZones( wxCommandEvent& event ){ ExportSetupToOtherCopperZones( event ); }
		void _wxFB_OnButtonOkClick( wxCommandEvent& event ){ OnButtonOkClick( event ); }
		void _wxFB_OnButtonCancelClick( wxCommandEvent& event ){ OnButtonCancelClick( event ); }
		void _wxFB_OnNetSortingOptionSelected( wxCommandEvent& event ){ OnNetSortingOptionSelected( event ); }
		
	
	protected:
		enum
		{
			 ID_RADIOBOX_GRID_SELECTION = 1000,
			wxID_PADS_IN_ZONE_OPTIONS,
			wxID_ANTIPAD_SIZE,
			wxID_COPPER_BRIDGE_VALUE,
			ID_RADIOBOX_OUTLINES_OPTION,
			wxID_ARC_APPROX,
			wxID_BUTTON_EXPORT,
			ID_NET_SORTING_OPTION,
			ID_TEXTCTRL_NETNAMES_FILTER,
			ID_NETNAME_SELECTION,
			ID_LAYER_CHOICE,
		};
		
		wxRadioBox* m_GridCtrl;
		wxRadioBox* m_PadInZoneOpt;
		wxStaticText* m_AntipadSizeText;
		wxTextCtrl* m_AntipadSizeValue;
		wxStaticText* m_CopperBridgeWidthText;
		wxTextCtrl* m_CopperWidthValue;
		
		wxRadioBox* m_OrientEdgesOpt;
		wxRadioBox* m_OutlineAppearanceCtrl;
		wxRadioBox* m_ArcApproximationOpt;
		wxCheckBox* m_ShowFilledAreasInSketchOpt;
		wxStaticText* m_ClearanceValueTitle;
		wxTextCtrl* m_ZoneClearanceCtrl;
		wxButton* m_ExportSetupButton;
		
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
		virtual void OnPadsInZoneClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void ExportSetupToOtherCopperZones( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnNetSortingOptionSelected( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		dialog_copper_zone_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Fill Zones Options"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 545,493 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~dialog_copper_zone_base();
	
};

#endif //__dialog_copper_zones_base__
