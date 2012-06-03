///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 17 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_plot_schematic_PDF_base__
#define __dialog_plot_schematic_PDF_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PLOT_SCHEMATIC_PDF_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PLOT_SCHEMATIC_PDF_BASE : public wxDialog 
{
	private:
	
	protected:
		wxRadioBox* m_SizeOption;
		
		wxRadioBox* m_PlotPDFColorOption;
		wxCheckBox* m_Plot_Sheet_Ref_Ctrl;
		
		wxButton* m_buttonPlotPage;
		wxButton* m_buttonPlotAll;
		wxButton* m_buttonClose;
		wxStaticText* m_defaultLineWidthTitle;
		wxTextCtrl* m_DefaultLineSizeCtrl;
		wxStaticText* m_staticText1;
		wxTextCtrl* m_MsgBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnPlotCurrent( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPlotAll( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PLOT_SCHEMATIC_PDF_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plot PDF"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 387,365 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PLOT_SCHEMATIC_PDF_BASE();
	
};

#endif //__dialog_plot_schematic_PDF_base__
