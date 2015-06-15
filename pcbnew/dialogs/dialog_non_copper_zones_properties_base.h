///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_NON_COPPER_ZONES_PROPERTIES_BASE_H__
#define __DIALOG_NON_COPPER_ZONES_PROPERTIES_BASE_H__

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
#include <wx/radiobox.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnOkClick( wxCommandEvent& event ){ OnOkClick( event ); }
		
	
	protected:
		wxStaticText* m_staticTextLayerSelection;
		wxListView* m_LayerSelectionCtrl;
		wxRadioBox* m_OrientEdgesOpt;
		wxRadioBox* m_OutlineAppearanceCtrl;
		wxStaticText* m_MinThicknessValueTitle;
		wxTextCtrl* m_ZoneMinThicknessCtrl;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizerButtons;
		wxButton* m_sdbSizerButtonsOK;
		wxButton* m_sdbSizerButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Non Copper Zones Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 369,317 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxSUNKEN_BORDER ); 
		~DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE();
	
};

#endif //__DIALOG_NON_COPPER_ZONES_PROPERTIES_BASE_H__
