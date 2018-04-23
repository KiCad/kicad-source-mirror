///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 19 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_GENDRILL_BASE_H__
#define __DIALOG_GENDRILL_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
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
		wxStaticText* staticTextOutputDir;
		wxTextCtrl* m_outputDirectoryName;
		wxBitmapButton* m_browseButton;
		wxRadioBox* m_rbFileFormat;
		wxRadioBox* m_Choice_Unit;
		wxRadioBox* m_Choice_Zeros_Format;
		wxStaticText* m_staticTextTitle;
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
		wxStaticText* staticTextPlatedPads;
		wxStaticText* m_PlatedPadsCountInfoMsg;
		wxStaticText* staticTextNonPlatedPads;
		wxStaticText* m_NotPlatedPadsCountInfoMsg;
		wxStaticText* staticTextThroughVias;
		wxStaticText* m_ThroughViasInfoMsg;
		wxStaticText* staticTextMicroVias;
		wxStaticText* m_MicroViasInfoMsg;
		wxStaticText* staticTextBuriedVias;
		wxStaticText* m_BuriedViasInfoMsg;
		wxButton* m_buttonDrill;
		wxButton* m_buttonMap;
		wxButton* m_buttonReport;
		wxButton* m_CancelButton;
		wxTextCtrl* m_messagesBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFileFormatSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelDrillUnitsSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSelZerosFmtSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenDrillFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenMapFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGenReportFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_GENDRILL_BASE( wxWindow* parent, wxWindowID id = wxID_CANCEL, const wxString& title = _("Generate Drill Files"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_GENDRILL_BASE();
	
};

#endif //__DIALOG_GENDRILL_BASE_H__
