///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar 19 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GENDRILL_BASE_H__
#define __DIALOG_GENDRILL_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GENDRILL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GENDRILL_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxRadioBox* m_Choice_Unit;
		wxRadioBox* m_Choice_Zeros_Format;
		wxRadioBox* m_Choice_Precision;
		wxRadioBox* m_Choice_Drill_Offset;
		wxRadioBox* m_Choice_Drill_Map;
		wxRadioBox* m_Choice_Drill_Report;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_PenSpeed;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_PenNum;
		wxCheckBox* m_Check_Mirror;
		wxCheckBox* m_Check_Minimal;
		wxStaticBoxSizer* m_DefaultViasDrillSizer;
		wxStaticText* m_ViaDrillValue;
		wxStaticBoxSizer* m_MicroViasDrillSizer;
		wxStaticText* m_MicroViaDrillValue;
		wxStaticText* m_PlatedPadsCountInfoMsg;
		wxStaticText* m_NotPlatedPadsCountInfoMsg;
		wxStaticText* m_ThroughViasInfoMsg;
		wxStaticText* m_MicroViasInfoMsg;
		wxStaticText* m_BuriedViasInfoMsg;
		wxButton* m_OkButton;
		wxButton* m_CancelButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSelDrillUnitsSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelZerosFmtSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Drill Files Generation"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GENDRILL_BASE();
	
};

#endif //__DIALOG_GENDRILL_BASE_H__
