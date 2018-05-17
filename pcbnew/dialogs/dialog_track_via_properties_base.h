///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_TRACK_VIA_PROPERTIES_BASE_H__
#define __DIALOG_TRACK_VIA_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;
class TEXT_CTRL_EVAL;
class WIDGET_NET_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/bmpcbox.h>
#include <wx/choice.h>
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
		wxStaticText* m_staticText24;
		WIDGET_NET_SELECTOR* m_NetComboBox;
		wxCheckBox* m_lockedCbox;
		wxStaticBoxSizer* m_sbTrackSizer;
		wxStaticText* m_TrackStartXLabel;
		TEXT_CTRL_EVAL* m_TrackStartXCtrl;
		wxStaticText* m_TrackStartXUnit;
		wxStaticText* m_TrackStartYLabel;
		TEXT_CTRL_EVAL* m_TrackStartYCtrl;
		wxStaticText* m_TrackStartYUnit;
		wxStaticText* m_TrackEndXLabel;
		TEXT_CTRL_EVAL* m_TrackEndXCtrl;
		wxStaticText* m_TrackEndXUnit;
		wxStaticText* m_TrackEndYLabel;
		TEXT_CTRL_EVAL* m_TrackEndYCtrl;
		wxStaticText* m_TrackEndYUnit;
		wxStaticText* m_TrackWidthLabel;
		wxComboBox* m_TrackWidthCtrl;
		wxStaticText* m_TrackWidthUnit;
		wxCheckBox* m_trackNetclass;
		wxStaticText* m_TrackLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_TrackLayerCtrl;
		wxStaticBoxSizer* m_sbViaSizer;
		wxStaticText* m_ViaXLabel;
		TEXT_CTRL_EVAL* m_ViaXCtrl;
		wxStaticText* m_ViaXUnit;
		wxStaticText* m_ViaYLabel;
		TEXT_CTRL_EVAL* m_ViaYCtrl;
		wxStaticText* m_ViaYUnit;
		wxStaticText* m_ViaDiameterLabel;
		TEXT_CTRL_EVAL* m_ViaDiameterCtrl;
		wxStaticText* m_ViaDiameterUnit;
		wxStaticText* m_ViaDrillLabel;
		TEXT_CTRL_EVAL* m_ViaDrillCtrl;
		wxStaticText* m_ViaDrillUnit;
		wxStaticText* m_DesignRuleVias;
		wxChoice* m_DesignRuleViasCtrl;
		wxStaticText* m_DesignRuleViasUnit;
		wxStaticText* m_ViaTypeLabel;
		wxChoice* m_ViaTypeChoice;
		wxStaticText* m_ViaStartLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_ViaStartLayer;
		wxStaticText* m_ViaEndLayerLabel1;
		PCB_LAYER_BOX_SELECTOR* m_ViaEndLayer;
		wxCheckBox* m_viaNetclass;
		wxStdDialogButtonSizer* m_StdButtons;
		wxButton* m_StdButtonsOK;
		wxButton* m_StdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void onOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onTrackNetclassCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaNetclassCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Track & Via Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_TRACK_VIA_PROPERTIES_BASE();
	
};

#endif //__DIALOG_TRACK_VIA_PROPERTIES_BASE_H__
