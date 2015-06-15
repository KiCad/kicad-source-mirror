///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GENDRILL_BASE_H__
#define __DIALOG_GENDRILL_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_GEN_DRILL_FILE 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GENDRILL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GENDRILL_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxTextCtrl* m_outputDirectoryName;
		wxButton* m_buttonBrowse;
		wxRadioBox* m_Choice_Unit;
		wxRadioBox* m_Choice_Zeros_Format;
		wxStaticText* m_staticTextPrecision;
		wxRadioBox* m_Choice_Drill_Map;
		wxCheckBox* m_Check_Mirror;
		wxCheckBox* m_Check_Minimal;
		wxCheckBox* m_Check_Merge_PTH_NPTH;
		wxRadioBox* m_Choice_Drill_Offset;
		wxStaticBoxSizer* m_DefaultViasDrillSizer;
		wxStaticText* m_ViaDrillValue;
		wxStaticBoxSizer* m_MicroViasDrillSizer;
		wxStaticText* m_MicroViaDrillValue;
		wxStaticText* m_PlatedPadsCountInfoMsg;
		wxStaticText* m_NotPlatedPadsCountInfoMsg;
		wxStaticText* m_ThroughViasInfoMsg;
		wxStaticText* m_MicroViasInfoMsg;
		wxStaticText* m_BuriedViasInfoMsg;
		wxButton* m_buttonDrill;
		wxButton* m_buttonMap;
		wxButton* m_buttonReport;
		wxButton* m_CancelButton;
		wxTextCtrl* m_messagesBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelDrillUnitsSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelZerosFmtSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenDrillFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenMapFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenReportFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Drill Files Generation"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 506,471 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GENDRILL_BASE();
	
};

#endif //__DIALOG_GENDRILL_BASE_H__
