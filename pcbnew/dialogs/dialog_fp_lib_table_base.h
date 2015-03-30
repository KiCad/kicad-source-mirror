///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Mar  9 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_FP_LIB_TABLE_BASE_H__
#define __DIALOG_FP_LIB_TABLE_BASE_H__

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
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/aui/auibook.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FP_LIB_TABLE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FP_LIB_TABLE_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxAuiNotebook* m_auinotebook;
		wxPanel* m_global_panel;
		wxStaticText* m_staticText3;
		wxStaticText* m_GblTableFilename;
		wxGrid* m_global_grid;
		wxPanel* m_project_panel;
		wxStaticText* m_staticText4;
		wxStaticText* m_PrjTableFilename;
		wxGrid* m_project_grid;
		wxButton* m_buttonWizard;
		wxButton* m_append_button;
		wxButton* m_delete_button;
		wxButton* m_move_up_button;
		wxButton* m_move_down_button;
		wxButton* m_edit_options;
		wxGrid* m_path_subs_grid;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onCancelCaptionButtonClick( wxCloseEvent& event ) = 0;
		virtual void onKeyDown( wxKeyEvent& event ) = 0;
		virtual void pageChangedHandler( wxAuiNotebookEvent& event ) = 0;
		virtual void OnClickLibraryWizard( wxCommandEvent& event ) = 0;
		virtual void appendRowHandler( wxCommandEvent& event ) = 0;
		virtual void deleteRowHandler( wxCommandEvent& event ) = 0;
		virtual void moveUpHandler( wxCommandEvent& event ) = 0;
		virtual void moveDownHandler( wxCommandEvent& event ) = 0;
		virtual void optionsEditor( wxCommandEvent& event ) = 0;
		virtual void onCancelButtonClick( wxCommandEvent& event ) = 0;
		virtual void onOKButtonClick( wxCommandEvent& event ) = 0;
		
	
	public:
		
		DIALOG_FP_LIB_TABLE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("PCB Library Tables"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 634,600 ), long style = wxCAPTION|wxCLOSE_BOX|wxDEFAULT_DIALOG_STYLE|wxMAXIMIZE_BOX|wxRESIZE_BORDER|wxSYSTEM_MENU ); 
		~DIALOG_FP_LIB_TABLE_BASE();
	
};

#endif //__DIALOG_FP_LIB_TABLE_BASE_H__
