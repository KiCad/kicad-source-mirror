///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_non_copper_zones_properties_base__
#define __dialog_non_copper_zones_properties_base__

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
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DialogNonCopperZonesPropertiesBase
///////////////////////////////////////////////////////////////////////////////
class DialogNonCopperZonesPropertiesBase : public wxDialog 
{
	DECLARE_EVENT_TABLE()
	private:
		
		// Private event handlers
		void _wxFB_OnOkClick( wxCommandEvent& event ){ OnOkClick( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		
	
	protected:
		wxRadioBox* m_FillModeCtrl;
		wxStaticText* m_MinThicknessValueTitle;
		wxTextCtrl* m_ZoneMinThicknessCtrl;
		wxRadioBox* m_OrientEdgesOpt;
		wxRadioBox* m_OutlineAppearanceCtrl;
		wxButton* m_buttonOk;
		wxButton* m_buttonCancel;
		wxStaticText* m_staticTextLayerSelection;
		wxListBox* m_LayerSelectionCtrl;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ){ event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		DialogNonCopperZonesPropertiesBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Non Copper Zones Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 416,287 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxFULL_REPAINT_ON_RESIZE|wxSUNKEN_BORDER );
		~DialogNonCopperZonesPropertiesBase();
	
};

#endif //__dialog_non_copper_zones_properties_base__
