///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 18 2010)
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
			ID_ALLOW_PRINT_PAD_ON_SILKSCREEN = 1000,
			ID_PRINT_REF,
			ID_MIROR_OPT,
			ID_SAVE_OPT_PLOT,
			ID_CREATE_DRILL_FILE,
		};
		
		wxStaticBoxSizer* m_CopperLayersBoxSizer;
		wxStaticBoxSizer* m_TechnicalLayersBoxSizer;
		wxCheckBox* m_useGerberExtensions;
		wxCheckBox* m_excludeEdgeLayerOpt;
		wxCheckBox* m_subtractMaskFromSilk;
		wxCheckBox* m_plotSheetRef;
		wxCheckBox* m_plotPads_on_Silkscreen;
		wxCheckBox* m_plotModuleValueOpt;
		wxCheckBox* m_plotModuleRefOpt;
		wxCheckBox* m_plotTextOther;
		wxCheckBox* m_plotInvisibleText;
		wxRadioBox* m_drillShapeOpt;
		wxRadioBox* m_scaleOpt;
		wxRadioBox* m_plotModeOpt;
		wxRadioBox* m_choicePlotOffset;
		wxRadioBox* m_plotFormatOpt;
		wxStaticText* m_textPenSize;
		wxTextCtrl* m_HPGLPenSizeOpt;
		wxStaticText* m_staticText3;
		wxTextCtrl* m_HPGLPenSpeedOpt;
		wxStaticText* m_textPenOvr;
		wxTextCtrl* m_HPGLPenOverlayOpt;
		wxCheckBox* m_plotPSNegativeOpt;
		wxCheckBox* m_plotMirrorOpt;
		wxCheckBox* m_plotNoViaOnMaskOpt;
		wxStaticText* m_staticText6;
		wxTextCtrl* m_linesWidth;
		
		wxStaticText* m_staticText7;
		wxTextCtrl* m_fineAdjustXscaleOpt;
		wxStaticText* m_staticText8;
		wxTextCtrl* m_fineAdjustYscaleOpt;
		
		wxButton* m_plotButton;
		wxButton* m_buttonSaveOpt;
		wxButton* m_buttonDrill;
		wxButton* m_buttonQuit;
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		wxButton* m_browseButton;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_messagesBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void SetPlotFormat( wxCommandEvent& event ) { event.Skip(); }
		virtual void Plot( wxCommandEvent& event ) { event.Skip(); }
		virtual void applyPlotSettings( wxCommandEvent& event ) { event.Skip(); }
		virtual void CreateDrillFile( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnQuit( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PLOT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plot"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 571,616 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PLOT_BASE();
	
};

#endif //__dialog_plot_base__
