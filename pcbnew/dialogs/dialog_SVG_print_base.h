///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May  6 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_SVG_PRINT_BASE_H__
#define __DIALOG_SVG_PRINT_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;
class WX_HTML_REPORT_PANEL;

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
#include <wx/checklst.h>
#include <wx/statbox.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SVG_PRINT_base
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SVG_PRINT_base : public DIALOG_SHIM
{
	private:
	
	protected:
		enum
		{
			wxID_PRINT_BOARD = 1000
		};
		
		wxStaticText* m_staticTextDir;
		wxTextCtrl* m_outputDirectoryName;
		wxButton* m_browseButton;
		wxStaticText* m_staticTextCopperLayers;
		wxCheckListBox* m_CopperLayersList;
		wxStaticText* m_staticTextTechLayers;
		wxCheckListBox* m_TechnicalLayersList;
		wxStaticText* m_TextPenWidth;
		wxTextCtrl* m_DialogDefaultPenSize;
		wxRadioBox* m_ModeColorOption;
		wxRadioBox* m_rbSvgPageSizeOpt;
		wxCheckBox* m_PrintBoardEdgesCtrl;
		wxCheckBox* m_printMirrorOpt;
		wxRadioBox* m_rbFileOpt;
		wxButton* m_buttonCreateFile;
		wxButton* m_buttonQuit;
		WX_HTML_REPORT_PANEL* m_messagesPanel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseWindow( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonPlot( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnButtonCloseClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_SVG_PRINT_base( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export SVG file"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 637,454 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_SVG_PRINT_base();
	
};

#endif //__DIALOG_SVG_PRINT_BASE_H__
