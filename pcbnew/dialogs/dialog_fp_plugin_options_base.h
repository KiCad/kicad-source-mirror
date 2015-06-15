///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
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
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/html/htmlwin.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FP_PLUGIN_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FP_PLUGIN_OPTIONS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxGrid* m_grid;
		wxButton* m_add_row;
		wxButton* m_delete_row;
		wxButton* m_move_up;
		wxButton* m_move_down;
		wxListBox* m_listbox;
		wxButton* m_append_choice_button;
		wxStaticText* m_staticText1;
		wxHtmlWindow* m_html;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onCancelCaptionButtonClick( wxCloseEvent& event ) = 0;
		virtual void onAppendRow( wxMouseEvent& event ) = 0;
		virtual void onDeleteRow( wxMouseEvent& event ) = 0;
		virtual void onMoveUp( wxMouseEvent& event ) = 0;
		virtual void onMoveDown( wxMouseEvent& event ) = 0;
		virtual void onListBoxItemSelected( wxCommandEvent& event ) = 0;
		virtual void onListBoxItemDoubleClicked( wxCommandEvent& event ) = 0;
		virtual void onAppendOption( wxCommandEvent& event ) = 0;
		virtual void onCancelButtonClick( wxCommandEvent& event ) = 0;
		virtual void onOKButtonClick( wxCommandEvent& event ) = 0;
		
	
	public:
		
		DIALOG_FP_PLUGIN_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 678,342 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER ); 
		~DIALOG_FP_PLUGIN_OPTIONS_BASE();
	
};

#endif //__DIALOG_FP_PLUGIN_OPTIONS_BASE_H__
