///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PLOT_SCHEMATIC_BASE_H__
#define __DIALOG_PLOT_SCHEMATIC_BASE_H__

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
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/choice.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PLOT_SCHEMATIC_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PLOT_SCHEMATIC_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			wxID_PRINT_CURRENT = 1000,
			wxID_PRINT_ALL
		};
		
		wxStaticText* m_staticTextOutputDirectory;
		wxTextCtrl* m_outputDirectoryName;
		wxButton* m_browseButton;
		wxBoxSizer* m_optionsSizer;
		wxStaticBoxSizer* m_paperOptionsSizer;
		wxRadioBox* m_PaperSizeOption;
		wxStaticBoxSizer* m_paperHPGLSizer;
		wxStaticText* m_staticText4;
		wxChoice* m_HPGLPaperSizeOption;
		wxRadioBox* m_plotOriginOpt;
		wxStaticText* m_penHPLGWidthTitle;
		wxTextCtrl* m_penHPGLWidthCtrl;
		wxRadioBox* m_plotFormatOpt;
		wxStaticText* m_defaultLineWidthTitle;
		wxTextCtrl* m_DefaultLineSizeCtrl;
		wxRadioBox* m_ModeColorOption;
		wxCheckBox* m_PlotFrameRefOpt;
		wxBoxSizer* m_ButtonsSizer;
		wxButton* m_buttonPlotCurrent;
		wxButton* m_buttonPlotAll;
		wxButton* m_buttonQuit;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_MessagesBox;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHPGLPageSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPlotFormatSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPlotCurrentClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPlotAllClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCancelClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PLOT_SCHEMATIC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Plot Schematic"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PLOT_SCHEMATIC_BASE();
	
};

#endif //__DIALOG_PLOT_SCHEMATIC_BASE_H__
