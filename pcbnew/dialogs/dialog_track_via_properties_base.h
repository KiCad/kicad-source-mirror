///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <widgets/net_selector.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/bmpcbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TRACK_VIA_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TRACK_VIA_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_MainSizer;
		wxStaticBoxSizer* m_sbCommonSizer;
		wxStaticText* m_netSelectorLabel;
		NET_SELECTOR* m_netSelector;
		wxCheckBox* m_viaNotFree;
		wxStaticLine* m_staticline1;
		wxCheckBox* m_lockedCbox;
		wxStaticBoxSizer* m_sbTrackSizer;
		wxStaticText* m_TrackStartXLabel;
		wxTextCtrl* m_TrackStartXCtrl;
		wxStaticText* m_TrackStartXUnit;
		wxStaticText* m_TrackStartYLabel;
		wxTextCtrl* m_TrackStartYCtrl;
		wxStaticText* m_TrackStartYUnit;
		wxStaticText* m_TrackEndXLabel;
		wxTextCtrl* m_TrackEndXCtrl;
		wxStaticText* m_TrackEndXUnit;
		wxStaticText* m_TrackEndYLabel;
		wxTextCtrl* m_TrackEndYCtrl;
		wxStaticText* m_TrackEndYUnit;
		wxStaticText* m_DesignRuleWidths;
		wxChoice* m_DesignRuleWidthsCtrl;
		wxStaticText* m_DesignRuleWidthsUnits;
		wxStaticText* m_TrackWidthLabel;
		wxTextCtrl* m_TrackWidthCtrl;
		wxStaticText* m_TrackWidthUnit;
		wxCheckBox* m_trackNetclass;
		wxStaticText* m_TrackLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_TrackLayerCtrl;
		wxStaticBoxSizer* m_sbViaSizer;
		wxStaticText* m_ViaXLabel;
		wxTextCtrl* m_ViaXCtrl;
		wxStaticText* m_ViaXUnit;
		wxStaticText* m_ViaYLabel;
		wxTextCtrl* m_ViaYCtrl;
		wxStaticText* m_ViaYUnit;
		wxStaticText* m_DesignRuleVias;
		wxChoice* m_DesignRuleViasCtrl;
		wxStaticText* m_DesignRuleViasUnit;
		wxStaticText* m_ViaDiameterLabel;
		wxTextCtrl* m_ViaDiameterCtrl;
		wxStaticText* m_ViaDiameterUnit;
		wxStaticText* m_ViaDrillLabel;
		wxTextCtrl* m_ViaDrillCtrl;
		wxStaticText* m_ViaDrillUnit;
		wxCheckBox* m_viaNetclass;
		wxStaticText* m_ViaTypeLabel;
		wxChoice* m_ViaTypeChoice;
		wxStaticText* m_ViaStartLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_ViaStartLayer;
		wxStaticText* m_ViaEndLayerLabel1;
		PCB_LAYER_BOX_SELECTOR* m_ViaEndLayer;
		wxStaticText* m_annularRingsLabel;
		wxChoice* m_annularRingsCtrl;
		wxStdDialogButtonSizer* m_StdButtons;
		wxButton* m_StdButtonsOK;
		wxButton* m_StdButtonsCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onViaNotFreeClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWidthSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onWidthEdit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTrackNetclassCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaEdit( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaNetclassCheck( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Track & Via Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU );
		~DIALOG_TRACK_VIA_PROPERTIES_BASE();

};

