///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PRINT_USING_PRINTER_BASE_H__
#define __DIALOG_PRINT_USING_PRINTER_BASE_H__

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
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PRINT_USING_PRINTER_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PRINT_USING_PRINTER_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			wxID_FRAME_SEL = 1000,
			wxID_PRINT_MODE,
			wxID_PAGE_MODE,
			wxID_PRINT_OPTIONS,
			wxID_PRINT_ALL
		};
		
		wxStaticText* m_staticText4;
		wxCheckListBox* m_CopperLayersList;
		wxStaticText* m_staticText5;
		wxCheckListBox* m_TechnicalLayersList;
		wxCheckBox* m_Exclude_Edges_Pcb;
		wxRadioBox* m_ScaleOption;
		wxStaticText* m_FineAdjustXscaleTitle;
		wxTextCtrl* m_FineAdjustXscaleOpt;
		wxStaticText* m_FineAdjustYscaleTitle;
		wxTextCtrl* m_FineAdjustYscaleOpt;
		wxStaticText* m_TextPenWidth;
		wxTextCtrl* m_DialogPenWidth;
		wxCheckBox* m_Print_Sheet_Ref;
		wxCheckBox* m_Print_Mirror;
		wxRadioBox* m_Drill_Shape_Opt;
		wxRadioBox* m_ModeColorOption;
		wxRadioBox* m_PagesOption;
		wxButton* m_buttonOption;
		wxButton* m_buttonPreview;
		wxButton* m_buttonPrint;
		wxButton* m_buttonQuit;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnScaleSelectionClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPrintPreview( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPrintButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PRINT_USING_PRINTER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Print"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 739,373 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PRINT_USING_PRINTER_BASE();
	
};

#endif //__DIALOG_PRINT_USING_PRINTER_BASE_H__
