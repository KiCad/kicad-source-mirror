///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 17 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PRINT_PCBNEW_BASE_H__
#define __DIALOG_PRINT_PCBNEW_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checklst.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PRINT_PCBNEW_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PRINT_PCBNEW_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			wxID_FRAME_SEL = 1000,
			wxID_PAGE_MODE,
			wxID_PRINT_OPTIONS
		};
		
		wxStaticText* m_staticText4;
		wxCheckListBox* m_CopperLayersList;
		wxStaticText* m_staticText5;
		wxCheckListBox* m_TechnicalLayersList;
		wxButton* m_buttonSelectAll;
		wxButton* m_buttonDeselectAll;
		wxCheckBox* m_Exclude_Edges_Pcb;
		wxStaticText* m_penWidthLabel;
		wxTextCtrl* m_penWidthCtrl;
		wxStaticText* m_penWidthUnits;
		wxStaticText* m_drillMarksLabel;
		wxChoice* m_drillMarksChoice;
		wxStaticText* m_outputModeLabel;
		wxChoice* m_outputMode;
		wxCheckBox* m_Print_Sheet_Ref;
		wxCheckBox* m_Print_Mirror;
		wxRadioBox* m_PagesOption;
		wxRadioButton* m_scale1;
		wxRadioButton* m_scaleFit;
		wxRadioButton* m_scaleCustom;
		wxTextCtrl* m_scaleCustomText;
		wxStaticLine* m_staticline1;
		wxButton* m_buttonOption;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Apply;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnSelectAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeselectAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSetCustomScale( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPrintPreview( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPrintButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PRINT_PCBNEW_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Print"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 700,550 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PRINT_PCBNEW_BASE();
	
};

#endif //__DIALOG_PRINT_PCBNEW_BASE_H__
