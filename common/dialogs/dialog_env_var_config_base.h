///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_ENV_VAR_CONFIG_BASE_H__
#define __DIALOG_ENV_VAR_CONFIG_BASE_H__

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
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ENV_VAR_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ENV_VAR_CONFIG_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGrid* m_grid;
		wxButton* m_buttonAdd;
		wxButton* m_buttonDelete;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxButton* m_sdbSizerHelp;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnAddRow( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDeleteSelectedRows( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelpRequest( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_ENV_VAR_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Path Configuration"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 363,177 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_ENV_VAR_CONFIG_BASE();
	
};

#endif //__DIALOG_ENV_VAR_CONFIG_BASE_H__
