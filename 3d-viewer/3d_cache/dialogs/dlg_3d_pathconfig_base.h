///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 21 2016)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DLG_3D_PATHCONFIG_BASE_H__
#define __DLG_3D_PATHCONFIG_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DLG_3D_PATH_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class DLG_3D_PATH_CONFIG_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGrid* m_EnvVars;
		wxButton* m_btnEnvCfg;
		wxStaticLine* m_staticline2;
		wxGrid* m_Aliases;
		wxButton* m_btnAddAlias;
		wxButton* m_btnDelAlias;
		wxButton* m_btnMoveUp;
		wxButton* m_btnMoveDown;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;
		wxButton* m_sdbSizer2Help;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnConfigEnvVar( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDelAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAliasMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAliasMoveDown( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DLG_3D_PATH_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("3D Search Path Configuration"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 619,319 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DLG_3D_PATH_CONFIG_BASE();
	
};

#endif //__DLG_3D_PATHCONFIG_BASE_H__
