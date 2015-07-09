///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_TRACK_VIA_PROPERTIES_BASE_H__
#define __DIALOG_TRACK_VIA_PROPERTIES_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class PCB_LAYER_BOX_SELECTOR;

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/bmpcbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TRACK_VIA_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TRACK_VIA_PROPERTIES_BASE : public wxDialog 
{
	private:
	
	protected:
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
		wxStaticLine* m_trackStaticLine;
		wxStaticText* m_TrackWidthLabel;
		wxTextCtrl* m_TrackWidthCtrl;
		wxStaticText* m_TrackWidthUnit;
		wxCheckBox* m_trackNetclass;
		wxStaticText* m_TrackLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_TrackLayerCtrl;
		wxStaticText* m_mainSizerAccessor;
		wxStaticText* m_ViaXLabel;
		wxTextCtrl* m_ViaXCtrl;
		wxStaticText* m_ViaXUnit;
		wxStaticText* m_ViaYLabel;
		wxTextCtrl* m_ViaYCtrl;
		wxStaticText* m_ViaYUnit;
		wxStaticLine* m_viaStaticLine;
		wxStaticText* m_ViaDiameterLabel;
		wxTextCtrl* m_ViaDiameterCtrl;
		wxStaticText* m_ViaDiameterUnit;
		wxStaticText* m_ViaDrillLabel;
		wxTextCtrl* m_ViaDrillCtrl;
		wxStaticText* m_ViaDrillUnit;
		wxCheckBox* m_viaNetclass;
		wxStdDialogButtonSizer* m_StdButtons;
		wxButton* m_StdButtonsOK;
		wxButton* m_StdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void onTrackNetclassCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViaNetclassCheck( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_TRACK_VIA_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Track & Via Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 576,333 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_TRACK_VIA_PROPERTIES_BASE();
	
};

#endif //__DIALOG_TRACK_VIA_PROPERTIES_BASE_H__
