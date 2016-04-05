///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct  5 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DLG_3D_PATHCONFIG_BASE_H__
#define __DLG_3D_PATHCONFIG_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DLG_3D_PATH_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class DLG_3D_PATH_CONFIG_BASE : public wxDialog 
{
	private:
	
	protected:
		wxGrid* m_Aliases;
		wxButton* m_btnAddAlias;
		wxButton* m_btnDelAlias;
		wxButton* m_btnMoveUp;
		wxButton* m_btnMoveDown;
		wxButton* m_btnOK;
		wxButton* m_btnCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnAddAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDelAlias( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAliasMoveUp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAliasMoveDown( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DLG_3D_PATH_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("3D Search Path Configuration"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE ); 
		~DLG_3D_PATH_CONFIG_BASE();
	
};

#endif //__DLG_3D_PATHCONFIG_BASE_H__
