///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_COPPER_ZONES_BASE_H__
#define __DIALOG_COPPER_ZONES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class wxListView;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/listbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_ZONE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COPPER_ZONE_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnClose( wxCloseEvent& event ){ OnClose( event ); }
		void _wxFB_OnNetSortingOptionSelected( wxCommandEvent& event ){ OnNetSortingOptionSelected( event ); }
		void _wxFB_OnRunFiltersButtonClick( wxCommandEvent& event ){ OnRunFiltersButtonClick( event ); }
		void _wxFB_OnCornerSmoothingModeChoice( wxCommandEvent& event ){ OnCornerSmoothingModeChoice( event ); }
		void _wxFB_OnPadsInZoneClick( wxCommandEvent& event ){ OnPadsInZoneClick( event ); }
		void _wxFB_ExportSetupToOtherCopperZones( wxCommandEvent& event ){ ExportSetupToOtherCopperZones( event ); }
		void _wxFB_OnButtonCancelClick( wxCommandEvent& event ){ OnButtonCancelClick( event ); }
		void _wxFB_OnButtonOkClick( wxCommandEvent& event ){ OnButtonOkClick( event ); }
		
	
	protected:
		enum
		{
			ID_DIALOG_COPPER_ZONE_BASE = 1000,
			ID_NETNAME_SELECTION,
			ID_M_NETDISPLAYOPTION,
			ID_TEXTCTRL_NETNAMES_FILTER,
			wxID_APPLY_FILTERS,
			ID_CORNER_SMOOTHING,
			ID_M_CORNERSMOOTHINGCTRL,
			ID_M_PADINZONEOPT,
			wxID_ANTIPAD_SIZE,
			wxID_COPPER_BRIDGE_VALUE,
			ID_M_PRIORITYLEVELCTRL,
			ID_M_FILLMODECTRL,
			ID_M_ARCAPPROXIMATIONOPT,
			ID_M_ORIENTEDGESOPT,
			ID_M_OUTLINEAPPEARANCECTRL,
			wxID_BUTTON_EXPORT
		};
		
		wxBoxSizer* m_MainBoxSizer;
		wxStaticText* m_staticText17;
		wxListView* m_LayerSelectionCtrl;
		wxStaticText* m_staticText2;
		wxListBox* m_ListNetNameSelection;
		wxStaticText* m_staticText16;
		wxChoice* m_NetDisplayOption;
		wxStaticText* m_staticText5;
		wxTextCtrl* m_DoNotShowNetNameFilter;
		wxStaticText* m_staticText51;
		wxTextCtrl* m_ShowNetNameFilter;
		wxButton* m_buttonRunFilter;
		wxStaticText* m_ClearanceValueTitle;
		wxTextCtrl* m_ZoneClearanceCtrl;
		wxStaticText* m_MinThicknessValueTitle;
		wxTextCtrl* m_ZoneMinThicknessCtrl;
		wxStaticText* m_staticText151;
		wxChoice* m_cornerSmoothingChoice;
		wxStaticText* m_cornerSmoothingTitle;
		wxTextCtrl* m_cornerSmoothingCtrl;
		wxStaticText* m_staticText13;
		wxChoice* m_PadInZoneOpt;
		wxStaticText* m_AntipadSizeText;
		wxTextCtrl* m_AntipadSizeValue;
		wxStaticText* m_CopperBridgeWidthText;
		wxTextCtrl* m_CopperWidthValue;
		wxStaticText* m_staticText171;
		wxSpinCtrl* m_PriorityLevelCtrl;
		wxStaticText* m_staticText11;
		wxChoice* m_FillModeCtrl;
		wxStaticText* m_staticText12;
		wxChoice* m_ArcApproximationOpt;
		wxStaticText* m_staticText14;
		wxChoice* m_OrientEdgesOpt;
		wxStaticText* m_staticText15;
		wxChoice* m_OutlineAppearanceCtrl;
		wxButton* m_ExportSetupButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnNetSortingOptionSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunFiltersButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCornerSmoothingModeChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPadsInZoneClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void ExportSetupToOtherCopperZones( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id = ID_DIALOG_COPPER_ZONE_BASE, const wxString& title = _("Copper Zone Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 567,507 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_COPPER_ZONE_BASE();
	
};

#endif //__DIALOG_COPPER_ZONES_BASE_H__
