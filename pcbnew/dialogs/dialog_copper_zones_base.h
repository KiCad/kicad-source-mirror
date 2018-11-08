///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_COPPER_ZONES_BASE_H__
#define __DIALOG_COPPER_ZONES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COPPER_ZONE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COPPER_ZONE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			ID_DIALOG_COPPER_ZONE_BASE = 1000,
			ID_NETNAME_SELECTION,
			ID_TEXTCTRL_NETNAMES_FILTER,
			wxID_APPLY_FILTERS,
			ID_CORNER_SMOOTHING,
			ID_M_CORNERSMOOTHINGCTRL,
			ID_M_PRIORITYLEVELCTRL,
			ID_M_OUTLINEAPPEARANCECTRL,
			ID_M_PADINZONEOPT,
			wxID_ANTIPAD_SIZE,
			wxID_COPPER_BRIDGE_VALUE,
			wxID_BUTTON_EXPORT
		};
		
		wxBoxSizer* m_MainBoxSizer;
		wxDataViewListCtrl* m_layers;
		wxListBox* m_ListNetNameSelection;
		wxStaticText* m_staticTextDisplay;
		wxTextCtrl* m_DoNotShowNetNameFilter;
		wxStaticText* m_staticTextVFilter;
		wxTextCtrl* m_ShowNetNameFilter;
		wxButton* m_buttonRunFilter;
		wxCheckBox* m_showAllNetsOpt;
		wxCheckBox* m_sortByPadsOpt;
		wxBoxSizer* m_bNoNetWarning;
		wxStaticBitmap* m_bitmapNoNetWarning;
		wxStaticText* m_staticText18;
		wxCheckBox* m_constrainOutline;
		wxStaticText* m_staticTextSmoothing;
		wxChoice* m_cornerSmoothingChoice;
		wxStaticText* m_cornerRadiusLabel;
		wxTextCtrl* m_cornerRadiusCtrl;
		wxStaticText* m_cornerRadiusUnits;
		wxStaticText* m_staticTextPriorityLevel;
		wxSpinCtrl* m_PriorityLevelCtrl;
		wxStaticText* m_staticTextStyle;
		wxChoice* m_OutlineAppearanceCtrl;
		wxStaticText* m_clearanceLabel;
		wxTextCtrl* m_clearanceCtrl;
		wxStaticText* m_clearanceUnits;
		wxStaticText* m_minWidthLabel;
		wxTextCtrl* m_minWidthCtrl;
		wxStaticText* m_minWidthUnits;
		wxStaticText* m_connectionLabel;
		wxChoice* m_PadInZoneOpt;
		wxStaticText* m_antipadLabel;
		wxTextCtrl* m_antipadCtrl;
		wxStaticText* m_antipadUnits;
		wxStaticText* m_spokeWidthLabel;
		wxTextCtrl* m_spokeWidthCtrl;
		wxStaticText* m_spokeWidthUnits;
		wxButton* m_ExportSetupButton;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnLayerSelection( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnRunFiltersButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNetSortingOptionSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void ExportSetupToOtherCopperZones( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_COPPER_ZONE_BASE( wxWindow* parent, wxWindowID id = ID_DIALOG_COPPER_ZONE_BASE, const wxString& title = _("Copper Zone Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_COPPER_ZONE_BASE();
	
};

#endif //__DIALOG_COPPER_ZONES_BASE_H__
