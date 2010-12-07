///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __dialog_plot_base__
#define __dialog_plot_base__

#include <wx/intl.h>

#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/gdicmn.h>
#include <wx/checkbox.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PLOT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PLOT_BASE : public wxDialog 
{
	private:
	
	protected:
		enum
		{
			ID_USE_GERBER_EXTENSIONS = 1000,
			ID_ALLOW_PRINT_PAD_ON_SILKSCREEN,
			ID_PRINT_VALUE,
			ID_PRINT_REF,
			ID_PRINT_MODULE_TEXTS,
			ID_FORCE_PRINT_INVISIBLE_TEXT,
			ID_DRILL_SHAPE_OPT,
			ID_BROWSE_OUTPUT_DIRECTORY,
			ID_MIROR_OPT,
			ID_MASKVIA_OPT,
			ID_EXEC_PLOT,
			ID_SAVE_OPT_PLOT,
			ID_CREATE_DRILL_FILE,
		};
		
		wxStaticBoxSizer* m_CopperLayersBoxSizer;
		wxStaticBoxSizer* m_TechnicalLayersBoxSizer;
		wxCheckBox* m_Use_Gerber_Extensions;
		wxCheckBox* m_Exclude_Edges_Pcb;
		wxCheckBox* m_SubtractMaskFromSilk;
		wxCheckBox* m_Plot_Sheet_Ref;
		wxCheckBox* m_Plot_Pads_on_Silkscreen;
		wxCheckBox* m_Plot_Text_Value;
		wxCheckBox* m_Plot_Text_Ref;
		wxCheckBox* m_Plot_Text_Div;
		wxCheckBox* m_Plot_Invisible_Text;
		wxRadioBox* m_Drill_Shape_Opt;
		wxRadioBox* m_Scale_Opt;
		wxRadioBox* m_PlotModeOpt;
		wxRadioBox* m_Choice_Plot_Offset;
		wxRadioBox* m_PlotFormatOpt;
		wxStaticText* m_textPenSize;
		wxTextCtrl* m_HPGLPenSizeOpt;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_HPGLPenSpeedOpt;
		wxStaticText* m_textPenOvr;
		wxTextCtrl* m_HPGLPenOverlayOpt;
		wxCheckBox* m_Plot_PS_Negative;
		wxTextCtrl* m_OutputDirectory;
		wxButton* m_BrowseButton;
		wxCheckBox* m_PlotMirorOpt;
		wxCheckBox* m_PlotNoViaOnMaskOpt;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_LinesWidth;
		
		wxStaticText* m_staticText7;
		wxTextCtrl* m_FineAdjustXscaleOpt;
		wxStaticText* m_staticText8;
		wxTextCtrl* m_FineAdjustYscaleOpt;
		
		wxButton* m_PlotButton;
		wxButton* m_buttonSaveOpt;
		wxButton* m_buttonDrill;
		wxButton* m_buttonQuit;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_MessagesBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void SetPlotFormat( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void Plot( wxCommandEvent& event ) { event.Skip(); }
		virtual void SaveOptPlot( wxCommandEvent& event ) { event.Skip(); }
		virtual void CreateDrillFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PLOT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plot"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PLOT_BASE();
	
};

#endif //__dialog_plot_base__
