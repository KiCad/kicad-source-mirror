///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 30 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_CONFIGURE_PATHS_BASE_H__
#define __DIALOG_CONFIGURE_PATHS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_CONFIGURE_PATHS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_CONFIGURE_PATHS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		WX_GRID* m_EnvVars;
		wxBitmapButton* m_btnAddEnvVar;
		wxBitmapButton* m_btnDeleteEnvVar;
		wxStaticBoxSizer* m_sb3DSearchPaths;
		WX_GRID* m_SearchPaths;
		wxBitmapButton* m_btnAddSearchPath;
		wxBitmapButton* m_btnMoveUp;
		wxBitmapButton* m_btnMoveDown;
		wxBitmapButton* m_btnDeleteSearchPath;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxButton* m_sdbSizerHelp;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnGridCellChange( wxGridEvent& event ) { event.Skip(); }
		virtual void OnGridSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnAddEnvVar( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveEnvVar( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnGridCellRightClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnAddSearchPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchPathMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchPathMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteSearchPath( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_CONFIGURE_PATHS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure Paths"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_CONFIGURE_PATHS_BASE();
	
};

#endif //__DIALOG_CONFIGURE_PATHS_BASE_H__
