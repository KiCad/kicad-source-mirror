///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 30 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_FP_PLUGIN_OPTIONS_BASE_H__
#define __DIALOG_FP_PLUGIN_OPTIONS_BASE_H__

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
#include <wx/statbox.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FP_PLUGIN_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FP_PLUGIN_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGrid* m_grid1;
		wxButton* m_add_row;
		wxButton* m_delete_row;
		wxButton* m_move_up;
		wxButton* m_move_down;
		wxButton* m_button1;
		wxListCtrl* m_listCtrl1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onCancelButtonClick( wxCloseEvent& event ) = 0;
		virtual void onAddRow( wxCommandEvent& event ) = 0;
		virtual void onDeleteRow( wxCommandEvent& event ) = 0;
		virtual void onMoveUp( wxCommandEvent& event ) = 0;
		virtual void onMoveDown( wxCommandEvent& event ) = 0;
		virtual void onCancelButtonClick( wxCommandEvent& event ) = 0;
		virtual void onOKButtonClick( wxCommandEvent& event ) = 0;
		
	
	public:
		
		DIALOG_FP_PLUGIN_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER ); 
		~DIALOG_FP_PLUGIN_OPTIONS_BASE();
	
};

#endif //__DIALOG_FP_PLUGIN_OPTIONS_BASE_H__
