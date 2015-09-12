///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_COLOR_CONFIG_BASE_H__
#define __DIALOG_COLOR_CONFIG_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/statline.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_COLOR_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_COLOR_CONFIG_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxBoxSizer* m_mainBoxSizer;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerApply;
		wxButton* m_sdbSizerCancel;
	
	public:
		
		DIALOG_COLOR_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("EESchema Colors"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_COLOR_CONFIG_BASE();
	
};

#endif //__DIALOG_COLOR_CONFIG_BASE_H__
